#!/usr/bin/env python3

import argparse
import json
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from neo4j import GraphDatabase, basic_auth


HOST = "127.0.0.1"
AUTH = basic_auth("admin", "73@TuGraph")
LOGICAL_GRAPH = "default"
PHYSICAL_GRAPH_PREFIX = "default_s"
PHYSICAL_GRAPH_WIDTH = 2
PERF_LABEL = "ProxyRaftPerfNode"


class ManagedProcess:
    def __init__(self, name, args, cwd, stdout_path):
        self.name = name
        self.args = args
        self.cwd = cwd
        self.stdout_path = stdout_path
        self.stdout_file = None
        self.proc = None

    def start(self):
        self.stdout_path.parent.mkdir(parents=True, exist_ok=True)
        self.stdout_file = self.stdout_path.open("wb")
        self.proc = subprocess.Popen(
            self.args,
            cwd=str(self.cwd),
            stdout=self.stdout_file,
            stderr=subprocess.STDOUT,
        )

    def poll(self):
        return None if self.proc is None else self.proc.poll()

    def stop(self, timeout=10):
        if self.proc is None:
            return
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=timeout)
        if self.stdout_file is not None:
            self.stdout_file.close()
            self.stdout_file = None


def allocate_ports(count):
    ports = []
    sockets = []
    try:
        for _ in range(count):
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.bind((HOST, 0))
            sock.listen(1)
            ports.append(sock.getsockname()[1])
            sockets.append(sock)
    finally:
        for sock in sockets:
            sock.close()
    return ports


def wait_for_port(port, process, timeout):
    deadline = time.monotonic() + timeout
    last_error = None
    while time.monotonic() < deadline:
        if process is not None and process.poll() is not None:
            raise RuntimeError(
                "{} exited while waiting for port {}. See {}".format(
                    process.name, port, process.stdout_path
                )
            )
        try:
            with socket.create_connection((HOST, port), timeout=0.2):
                return
        except OSError as exc:
            last_error = exc
            time.sleep(0.1)
    raise TimeoutError("timed out waiting for {}:{}: {}".format(HOST, port, last_error))


def open_driver(port, timeout=5):
    return GraphDatabase.driver(
        "bolt://{}:{}".format(HOST, port),
        auth=AUTH,
        encrypted=False,
        connection_timeout=timeout,
        max_connection_lifetime=60,
    )


def run_query(driver, query, **params):
    with driver.session(database=LOGICAL_GRAPH) as session:
        result = session.run(query, **params)
        result.consume()


def single_value(driver, query, key, **params):
    with driver.session(database=LOGICAL_GRAPH) as session:
        record = session.run(query, **params).single()
    if record is None:
        raise RuntimeError("query returned no rows: {}".format(query))
    return record[key]


def graph_name(shard_id):
    return "{}{:0{}d}".format(PHYSICAL_GRAPH_PREFIX, shard_id, PHYSICAL_GRAPH_WIDTH)


def raft_members(servers, graph):
    members = []
    for index, server in enumerate(servers):
        members.append(
            {
                "node_id": index + 1,
                "ip": HOST,
                "bolt_port": server["bolt_port"],
                "raft_port": server["raft_port"],
                "graph": graph,
            }
        )
    return members


def create_raft_graphs(server_drivers, servers, shard_count):
    for shard_id in range(shard_count):
        graph = graph_name(shard_id)
        members = raft_members(servers, graph)
        for driver in server_drivers:
            run_query(
                driver,
                "CALL dbms.graph.createGraphWithRaft($graph_name, $members)",
                graph_name=graph,
                members=members,
            )


def wait_for_leader(server_drivers, graph, timeout):
    deadline = time.monotonic() + timeout
    query = (
        "CALL dbms.graph.getRaftNodeInfos($graph_name) "
        "YIELD node_id, is_leader "
        "RETURN node_id, is_leader"
    )
    last_error = None
    while time.monotonic() < deadline:
        for driver in server_drivers:
            try:
                with driver.session(database=LOGICAL_GRAPH) as session:
                    rows = list(session.run(query, graph_name=graph))
                leaders = [row["node_id"] for row in rows if row["is_leader"]]
                if len(leaders) == 1:
                    return leaders[0]
            except Exception as exc:
                last_error = exc
        time.sleep(0.2)
    raise TimeoutError(
        "timed out waiting for raft leader for {}: {}".format(graph, last_error)
    )


def stable_hash(value):
    result = 14695981039346656037
    for byte in value.encode("utf-8"):
        result ^= byte
        result = (result * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return result


def shard_for_key(key, shard_count):
    return stable_hash(key) % shard_count


def find_shard_keys(shard_count):
    keys = [None] * shard_count
    candidate = 0
    while any(key is None for key in keys):
        key = "proxy-raft-perf-{}".format(candidate)
        shard = shard_for_key(key, shard_count)
        if keys[shard] is None:
            keys[shard] = key
        candidate += 1
    return keys


def time_parallel(total_ops, thread_count, worker):
    if total_ops <= 0:
        return 0.0
    start = time.perf_counter()
    errors = []
    lock = threading.Lock()

    def run_slice(thread_id, begin, end):
        try:
            worker(thread_id, begin, end)
        except Exception as exc:
            with lock:
                errors.append(exc)

    futures = []
    with ThreadPoolExecutor(max_workers=thread_count) as executor:
        for thread_id in range(thread_count):
            begin = (total_ops * thread_id) // thread_count
            end = (total_ops * (thread_id + 1)) // thread_count
            if begin == end:
                continue
            futures.append(executor.submit(run_slice, thread_id, begin, end))
        for future in as_completed(futures):
            future.result()
    if errors:
        raise errors[0]
    return time.perf_counter() - start


def run_warmup(driver, shard_keys, warmup_ops):
    query = "RETURN $value AS value"
    for i in range(warmup_ops):
        shard = i % len(shard_keys)
        with driver.session(database=LOGICAL_GRAPH) as session:
            value = session.run(
                query,
                _shard_key_=shard_keys[shard],
                value=i,
            ).single()["value"]
        if value != i:
            raise AssertionError("unexpected warmup value: {}".format(value))


def run_write_perf(driver, shard_keys, write_ops, thread_count):
    query = (
        "CREATE (n:{} "
        "{{id: $id, shard: $shard, worker: $worker, payload: $payload}})"
    ).format(PERF_LABEL)

    def worker(thread_id, begin, end):
        with driver.session(database=LOGICAL_GRAPH) as session:
            for op in range(begin, end):
                shard = op % len(shard_keys)
                session.run(
                    query,
                    _shard_key_=shard_keys[shard],
                    id=op,
                    shard=shard,
                    worker=thread_id,
                    payload="payload-{}".format(op % 97),
                ).consume()

    seconds = time_parallel(write_ops, thread_count, worker)
    return write_ops / seconds if seconds > 0 else 0.0


def run_read_perf(driver, shard_keys, read_ops, thread_count):
    query = "MATCH (n:{}) RETURN count(n) AS count".format(PERF_LABEL)

    def worker(thread_id, begin, end):
        del thread_id
        with driver.session(database=LOGICAL_GRAPH) as session:
            for op in range(begin, end):
                shard = op % len(shard_keys)
                count = session.run(
                    query,
                    _shard_key_=shard_keys[shard],
                ).single()["count"]
                if count < 0:
                    raise AssertionError("negative count from shard {}".format(shard))

    seconds = time_parallel(read_ops, thread_count, worker)
    return read_ops / seconds if seconds > 0 else 0.0


def verify_counts(driver, shard_keys, write_ops):
    query = "MATCH (n:{}) RETURN count(n) AS count".format(PERF_LABEL)
    observed = []
    for shard, key in enumerate(shard_keys):
        count = single_value(driver, query, "count", _shard_key_=key)
        expected = write_ops // len(shard_keys)
        if shard < write_ops % len(shard_keys):
            expected += 1
        if count != expected:
            raise AssertionError(
                "unexpected count on shard {}: expected {}, observed {}".format(
                    shard, expected, count
                )
            )
        observed.append(count)
    return observed


def build_proxy_backend_spec(servers, shard_count):
    replicas = []
    for index, server in enumerate(servers):
        replicas.append(
            "{}@{}:{}:{}".format(
                index + 1, HOST, server["bolt_port"], server["raft_port"]
            )
        )
    return "0-{}={}".format(shard_count - 1, ",".join(replicas))


def start_cluster(args, work_dir, ports):
    server_ports = ports[:6]
    proxy_port = ports[6]
    servers = []
    processes = []
    server_drivers = []

    try:
        for index in range(3):
            server_dir = work_dir / "server{}".format(index + 1)
            data_dir = server_dir / "data"
            log_dir = server_dir / "log"
            data_dir.mkdir(parents=True, exist_ok=True)
            log_dir.mkdir(parents=True, exist_ok=True)
            bolt_port = server_ports[index * 2]
            raft_port = server_ports[index * 2 + 1]
            server = {
                "bolt_port": bolt_port,
                "raft_port": raft_port,
                "dir": server_dir,
            }
            servers.append(server)
            command = [
                str(args.build_dir / "lgraph_server"),
                "--mode=run",
                "--host={}".format(HOST),
                "--bolt_port={}".format(bolt_port),
                "--raft_port={}".format(raft_port),
                "--data_path={}".format(data_dir),
                "--log_path={}".format(log_dir),
                "--pid_file={}".format(server_dir / "lgraph.pid"),
                "--log_level={}".format(args.log_level),
                "--bolt_io_thread_num={}".format(args.server_io_threads),
                "--bolt_worker_thread_num={}".format(args.server_worker_threads),
                "--max_bolt_connections={}".format(args.max_bolt_connections),
                "--graph_block_cache={}".format(args.graph_block_cache),
                "--graph_row_cache={}".format(args.graph_row_cache),
                "--raft_log_block_cache={}".format(args.raft_log_block_cache),
                "--raft_scheduler_shards={}".format(args.raft_scheduler_shards),
                "--assistant_thread_num={}".format(args.assistant_threads),
            ]
            process = ManagedProcess(
                "lgraph_server_{}".format(index + 1),
                command,
                args.build_dir,
                server_dir / "stdout.log",
            )
            process.start()
            processes.append(process)

        for process, server in zip(processes, servers):
            wait_for_port(server["bolt_port"], process, args.startup_timeout)
            wait_for_port(server["raft_port"], process, args.startup_timeout)

        server_drivers = [open_driver(server["bolt_port"]) for server in servers]
        for driver in server_drivers:
            value = single_value(driver, "RETURN 1 AS value", "value")
            if value != 1:
                raise AssertionError("unexpected connectivity value: {}".format(value))
        create_raft_graphs(server_drivers, servers, args.shards)
        leaders = {}
        for shard_id in range(args.shards):
            graph = graph_name(shard_id)
            leaders[graph] = wait_for_leader(
                server_drivers, graph, args.leader_timeout
            )

        proxy_dir = work_dir / "proxy"
        proxy_dir.mkdir(parents=True, exist_ok=True)
        proxy_command = [
            str(args.build_dir / "lgraph_proxy"),
            "--proxy_bolt_port={}".format(proxy_port),
            "--proxy_bolt_io_thread_num={}".format(args.proxy_io_threads),
            "--proxy_worker_thread_num={}".format(args.proxy_worker_threads),
            "--proxy_backend_max_connections_per_backend={}".format(
                args.backend_max_connections_per_backend
            ),
            "--proxy_backend_borrow_timeout_ms={}".format(
                args.backend_borrow_timeout_ms
            ),
            "--proxy_logical_graph={}".format(LOGICAL_GRAPH),
            "--proxy_physical_graph_prefix={}".format(PHYSICAL_GRAPH_PREFIX),
            "--proxy_shard_count={}".format(args.shards),
            "--proxy_shard_id_width={}".format(PHYSICAL_GRAPH_WIDTH),
            "--proxy_raft_backends={}".format(
                build_proxy_backend_spec(servers, args.shards)
            ),
            "--log_path={}".format(proxy_dir / "log"),
            "--log_level={}".format(args.log_level),
        ]
        proxy_process = ManagedProcess(
            "lgraph_proxy", proxy_command, args.build_dir, proxy_dir / "stdout.log"
        )
        proxy_process.start()
        processes.append(proxy_process)
        wait_for_port(proxy_port, proxy_process, args.startup_timeout)

        return {
            "servers": servers,
            "server_drivers": server_drivers,
            "proxy_port": proxy_port,
            "processes": processes,
            "leaders": leaders,
        }
    except Exception:
        for driver in server_drivers:
            driver.close()
        for process in reversed(processes):
            process.stop()
        raise


def validate_args(args):
    if args.shards < 2:
        raise ValueError("--shards must be at least 2")
    if args.threads < 1:
        raise ValueError("--threads must be at least 1")
    if args.write_ops < 0 or args.read_ops < 0 or args.warmup_ops < 0:
        raise ValueError("operation counts must be non-negative")
    if not (args.build_dir / "lgraph_server").exists():
        raise FileNotFoundError("missing {}".format(args.build_dir / "lgraph_server"))
    if not (args.build_dir / "lgraph_proxy").exists():
        raise FileNotFoundError("missing {}".format(args.build_dir / "lgraph_proxy"))


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=(
            "Start a 3-node raft cluster plus lgraph_proxy and run a neo4j "
            "Python driver performance smoke test through the proxy."
        )
    )
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--keep-data", action="store_true")
    parser.add_argument("--shards", type=int, default=4)
    parser.add_argument("--warmup-ops", type=int, default=16)
    parser.add_argument("--write-ops", type=int, default=200)
    parser.add_argument("--read-ops", type=int, default=80)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--min-write-qps", type=float, default=0.0)
    parser.add_argument("--min-read-qps", type=float, default=0.0)
    parser.add_argument("--startup-timeout", type=float, default=30.0)
    parser.add_argument("--leader-timeout", type=float, default=30.0)
    parser.add_argument("--log-level", default="info")
    parser.add_argument("--server-io-threads", type=int, default=1)
    parser.add_argument("--server-worker-threads", type=int, default=8)
    parser.add_argument("--proxy-io-threads", type=int, default=2)
    parser.add_argument("--proxy-worker-threads", type=int, default=8)
    parser.add_argument("--backend-max-connections-per-backend", type=int, default=64)
    parser.add_argument("--backend-borrow-timeout-ms", type=int, default=5000)
    parser.add_argument("--max-bolt-connections", type=int, default=1000)
    parser.add_argument("--graph-block-cache", type=int, default=64 * 1024 * 1024)
    parser.add_argument("--graph-row-cache", type=int, default=16 * 1024 * 1024)
    parser.add_argument("--raft-log-block-cache", type=int, default=32 * 1024 * 1024)
    parser.add_argument("--raft-scheduler-shards", type=int, default=4)
    parser.add_argument("--assistant-threads", type=int, default=2)
    parsed = parser.parse_args(argv)
    parsed.build_dir = parsed.build_dir.resolve()
    if parsed.work_dir is not None:
        parsed.work_dir = parsed.work_dir.resolve()
    validate_args(parsed)
    return parsed


def main(argv):
    args = parse_args(argv)
    root_work_dir = args.work_dir
    if root_work_dir is None:
        root_work_dir = Path(
            tempfile.mkdtemp(prefix="proxy_raft_perf_", dir=str(args.build_dir))
        )
        remove_work_dir = not args.keep_data
    else:
        if root_work_dir.exists():
            shutil.rmtree(root_work_dir)
        root_work_dir.mkdir(parents=True)
        remove_work_dir = not args.keep_data

    ports = allocate_ports(7)
    cluster = None
    proxy_driver = None
    try:
        setup_start = time.perf_counter()
        cluster = start_cluster(args, root_work_dir, ports)
        setup_seconds = time.perf_counter() - setup_start

        shard_keys = find_shard_keys(args.shards)
        proxy_driver = open_driver(cluster["proxy_port"])
        run_warmup(proxy_driver, shard_keys, args.warmup_ops)

        write_qps = run_write_perf(
            proxy_driver, shard_keys, args.write_ops, args.threads
        )
        counts = verify_counts(proxy_driver, shard_keys, args.write_ops)
        read_qps = run_read_perf(proxy_driver, shard_keys, args.read_ops, args.threads)

        if args.min_write_qps > 0 and write_qps < args.min_write_qps:
            raise AssertionError(
                "write throughput {:.2f} qps is lower than {:.2f}".format(
                    write_qps, args.min_write_qps
                )
            )
        if args.min_read_qps > 0 and read_qps < args.min_read_qps:
            raise AssertionError(
                "read throughput {:.2f} qps is lower than {:.2f}".format(
                    read_qps, args.min_read_qps
                )
            )

        summary = {
            "proxy_port": cluster["proxy_port"],
            "servers": [
                {
                    "node_id": index + 1,
                    "bolt_port": server["bolt_port"],
                    "raft_port": server["raft_port"],
                }
                for index, server in enumerate(cluster["servers"])
            ],
            "shards": args.shards,
            "leaders": cluster["leaders"],
            "write_ops": args.write_ops,
            "read_ops": args.read_ops,
            "threads": args.threads,
            "setup_seconds": round(setup_seconds, 3),
            "write_qps": round(write_qps, 2),
            "read_qps": round(read_qps, 2),
            "counts": counts,
            "work_dir": str(root_work_dir),
        }
        print(json.dumps(summary, indent=2, sort_keys=True))
        return 0
    finally:
        if proxy_driver is not None:
            proxy_driver.close()
        if cluster is not None:
            for driver in cluster["server_drivers"]:
                driver.close()
            for process in reversed(cluster["processes"]):
                process.stop()
        if remove_work_dir:
            shutil.rmtree(root_work_dir, ignore_errors=True)


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        sys.exit(130)
