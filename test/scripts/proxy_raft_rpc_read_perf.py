#!/usr/bin/env python3

import argparse
import json
import math
import multiprocessing as mp
import shutil
import sys
import tempfile
import time
from pathlib import Path

from neo4j import GraphDatabase

from proxy_raft_perf_integration import (
    AUTH,
    HOST,
    LOGICAL_GRAPH,
    allocate_ports,
    find_shard_keys,
    open_driver,
    start_cluster,
)

DEFAULT_CLIENT_NUM = 16
DEFAULT_PROXY_NUM = 1


def percentile(sorted_values, percent):
    if not sorted_values:
        return 0.0
    rank = (len(sorted_values) - 1) * percent / 100.0
    lower = int(math.floor(rank))
    upper = int(math.ceil(rank))
    if lower == upper:
        return sorted_values[lower]
    weight = rank - lower
    return sorted_values[lower] * (1.0 - weight) + sorted_values[upper] * weight


def run_one(session, query, shard_key, value, payload):
    params = {
        "value": value,
        "payload": payload,
    }
    if shard_key is not None:
        params["_shard_key_"] = shard_key
    record = session.run(query, **params).single()
    if record is None:
        raise AssertionError("read query returned no row")
    if record["value"] != value:
        raise AssertionError(
            "unexpected returned value: expected {}, observed {}".format(
                value, record["value"]
            )
        )
    if payload and record["payload"] != payload:
        raise AssertionError("unexpected returned payload")


def warmup_routes(routes, query, warmup_ops, payload):
    warmup_count = max(warmup_ops, len(routes))
    sessions = {}
    try:
        for op in range(warmup_count):
            route = routes[op % len(routes)]
            session_key = (id(route["driver"]), route["database"])
            session = sessions.get(session_key)
            if session is None:
                session = route["driver"].session(database=route["database"])
                sessions[session_key] = session
            run_one(session, query, route["shard_key"], op, payload)
    finally:
        for session in sessions.values():
            session.close()
    return warmup_count


def open_uri_driver(uri, pool_size=4):
    return GraphDatabase.driver(
        uri,
        auth=AUTH,
        encrypted=False,
        connection_timeout=5,
        max_connection_lifetime=60,
        max_connection_pool_size=pool_size,
    )


def process_worker(route_specs, query, read_ops, payload, worker_id, queue):
    target = route_specs[worker_id % len(route_specs)]
    shard_keys = target["shard_keys"]
    driver = open_uri_driver(target["uri"])
    latencies_ms = []
    try:
        with driver.session(database=target["database"]) as session:
            for op in range(read_ops):
                shard_key = shard_keys[op % len(shard_keys)]
                value = worker_id * read_ops + op
                op_start = time.perf_counter()
                run_one(session, query, shard_key, value, payload)
                latencies_ms.append((time.perf_counter() - op_start) * 1000.0)
        queue.put(
            {
                "ok": True,
                "latencies_ms": latencies_ms,
            }
        )
    except Exception as exc:
        queue.put({"ok": False, "error": repr(exc)})
    finally:
        driver.close()


def run_rpc_read_perf_processes(route_specs, query, read_ops, process_count, payload):
    round_robin_shards = any(len(route["shard_keys"]) > 1 for route in route_specs)
    base_ops = read_ops // process_count
    remainder = read_ops % process_count
    queue = mp.Queue()
    processes = []
    start = time.perf_counter()
    for worker_id in range(process_count):
        ops = base_ops + (1 if worker_id < remainder else 0)
        if ops == 0:
            continue
        proc = mp.Process(
            target=process_worker,
            args=(route_specs, query, ops, payload, worker_id, queue),
        )
        proc.start()
        processes.append(proc)

    latencies_ms = []
    errors = []
    for _ in processes:
        result = queue.get()
        if result["ok"]:
            latencies_ms.extend(result["latencies_ms"])
        else:
            errors.append(result["error"])
    for proc in processes:
        proc.join()

    elapsed_seconds = time.perf_counter() - start
    if errors:
        raise RuntimeError(errors[0])
    latencies_ms.sort()
    avg_latency = sum(latencies_ms) / len(latencies_ms) if latencies_ms else 0.0
    return {
        "seconds": elapsed_seconds,
        "qps": len(latencies_ms) / elapsed_seconds if elapsed_seconds > 0 else 0.0,
        "session_strategy": (
            "one_process_session_round_robin_shard_keys"
            if round_robin_shards
            else "one_process_session_pinned_route"
        ),
        "latency_ms": {
            "avg": avg_latency,
            "p50": percentile(latencies_ms, 50),
            "p95": percentile(latencies_ms, 95),
            "p99": percentile(latencies_ms, 99),
            "max": latencies_ms[-1] if latencies_ms else 0.0,
        },
    }


def round_perf(perf):
    return {
        "seconds": round(perf["seconds"], 3),
        "qps": round(perf["qps"], 2),
        "session_strategy": perf["session_strategy"],
        "latency_ms": {
            key: round(value, 3) for key, value in perf["latency_ms"].items()
        },
    }


def build_query(args):
    if args.payload_bytes > 0:
        return "RETURN $value AS value, $payload AS payload"
    return "RETURN $value AS value"


def build_proxy_routes(proxy_drivers, shard_keys):
    routes = []
    for driver in proxy_drivers:
        for shard_key in shard_keys:
            routes.append(
                {
                    "driver": driver,
                    "database": LOGICAL_GRAPH,
                    "shard_key": shard_key,
                }
            )
    return routes


def build_proxy_route_specs(proxy_ports, shard_keys):
    return [
        {
            "uri": "bolt://{}:{}".format(HOST, proxy_port),
            "database": LOGICAL_GRAPH,
            "shard_keys": shard_keys,
        }
        for proxy_port in proxy_ports
    ]


def build_direct_leader_routes(cluster):
    drivers = {}
    routes = []
    for shard_id in range(len(cluster["leaders"])):
        graph = "default_s{:02d}".format(shard_id)
        leader_node_id = cluster["leaders"][graph]
        driver = drivers.get(leader_node_id)
        if driver is None:
            server = cluster["servers"][leader_node_id - 1]
            driver = open_driver(server["bolt_port"])
            drivers[leader_node_id] = driver
        routes.append(
            {
                "driver": driver,
                "database": graph,
                "shard_key": None,
            }
        )
    return routes, list(drivers.values())


def build_direct_leader_route_specs(cluster):
    routes = []
    for shard_id in range(len(cluster["leaders"])):
        graph = "default_s{:02d}".format(shard_id)
        leader_node_id = cluster["leaders"][graph]
        server = cluster["servers"][leader_node_id - 1]
        routes.append(
            {
                "uri": "bolt://{}:{}".format(HOST, server["bolt_port"]),
                "database": graph,
                "shard_keys": [None],
            }
        )
    return routes


def validate_args(args):
    if args.shards < 2:
        raise ValueError("--shards must be at least 2")
    if args.read_ops < 1:
        raise ValueError("--read-ops must be at least 1")
    if args.warmup_ops < 0:
        raise ValueError("--warmup-ops must be non-negative")
    if args.payload_bytes < 0:
        raise ValueError("--payload-bytes must be non-negative")
    if args.client_num < 1:
        raise ValueError("--client-num must be at least 1")
    if args.proxy_num < 1:
        raise ValueError("--proxy-num must be at least 1")
    if not (args.build_dir / "lgraph_server").exists():
        raise FileNotFoundError("missing {}".format(args.build_dir / "lgraph_server"))
    if not (args.build_dir / "lgraph_proxy").exists():
        raise FileNotFoundError("missing {}".format(args.build_dir / "lgraph_proxy"))


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=(
            "Start one or more lgraph_proxy and three lgraph_server processes with "
            "multiple raft graphs, then benchmark pure read requests through "
            "the Neo4j Python driver. The measured phase sends only RETURN "
            "queries through proxy from multiprocess clients to focus on "
            "RPC/proxy request overhead."
        )
    )
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--keep-data", action="store_true")
    parser.add_argument("--shards", type=int, default=8)
    parser.add_argument("--warmup-ops", type=int, default=1000)
    parser.add_argument("--read-ops", type=int, default=10000)
    parser.add_argument("--client-num", type=int, default=DEFAULT_CLIENT_NUM)
    parser.add_argument("--proxy-num", type=int, default=DEFAULT_PROXY_NUM)
    parser.add_argument("--payload-bytes", type=int, default=0)
    parser.add_argument("--compare-direct", action="store_true")
    parser.add_argument("--min-qps", type=float, default=0.0)
    parser.add_argument("--startup-timeout", type=float, default=30.0)
    parser.add_argument("--leader-timeout", type=float, default=30.0)
    parser.add_argument("--log-level", default="error")
    parser.add_argument("--server-io-threads", type=int, default=1)
    parser.add_argument("--server-worker-threads", type=int, default=16)
    parser.add_argument("--proxy-io-threads", type=int, default=2)
    parser.add_argument("--proxy-worker-threads", type=int, default=8)
    parser.add_argument("--backend-max-connections-per-backend", type=int, default=128)
    parser.add_argument("--backend-borrow-timeout-ms", type=int, default=5000)
    parser.add_argument("--max-bolt-connections", type=int, default=2000)
    parser.add_argument("--graph-block-cache", type=int, default=64 * 1024 * 1024)
    parser.add_argument("--graph-row-cache", type=int, default=16 * 1024 * 1024)
    parser.add_argument("--raft-log-block-cache", type=int, default=32 * 1024 * 1024)
    parser.add_argument("--raft-scheduler-shards", type=int, default=4)
    parser.add_argument("--assistant-threads", type=int, default=2)
    args = parser.parse_args(argv)
    args.build_dir = args.build_dir.resolve()
    if args.work_dir is not None:
        args.work_dir = args.work_dir.resolve()
    validate_args(args)
    return args


def main(argv):
    args = parse_args(argv)
    if args.work_dir is None:
        work_dir = Path(
            tempfile.mkdtemp(prefix="proxy_raft_rpc_read_perf_", dir=str(args.build_dir))
        )
        remove_work_dir = not args.keep_data
    else:
        work_dir = args.work_dir
        if work_dir.exists():
            shutil.rmtree(work_dir)
        work_dir.mkdir(parents=True)
        remove_work_dir = not args.keep_data

    cluster = None
    proxy_drivers = []
    direct_drivers = []
    try:
        setup_start = time.perf_counter()
        cluster = start_cluster(args, work_dir, allocate_ports(6 + args.proxy_num))
        setup_seconds = time.perf_counter() - setup_start

        shard_keys = find_shard_keys(args.shards)
        query = build_query(args)
        payload = "x" * args.payload_bytes
        proxy_drivers = [open_driver(port) for port in cluster["proxy_ports"]]
        proxy_routes = build_proxy_routes(proxy_drivers, shard_keys)
        proxy_route_specs = build_proxy_route_specs(cluster["proxy_ports"], shard_keys)
        actual_warmup_ops = warmup_routes(proxy_routes, query, args.warmup_ops, payload)
        proxy_perf = run_rpc_read_perf_processes(
            proxy_route_specs, query, args.read_ops, args.client_num, payload
        )
        direct_perf = None
        if args.compare_direct:
            direct_routes, direct_drivers = build_direct_leader_routes(cluster)
            direct_route_specs = build_direct_leader_route_specs(cluster)
            warmup_routes(direct_routes, query, args.warmup_ops, payload)
            direct_perf = run_rpc_read_perf_processes(
                direct_route_specs, query, args.read_ops, args.client_num, payload
            )
        if args.min_qps > 0 and proxy_perf["qps"] < args.min_qps:
            raise AssertionError(
                "read throughput {:.2f} qps is lower than {:.2f}".format(
                    proxy_perf["qps"], args.min_qps
                )
            )

        summary = {
            "benchmark": "proxy_raft_rpc_read_perf",
            "query": query,
            "payload_bytes": args.payload_bytes,
            "proxy_ports": cluster["proxy_ports"],
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
            "warmup_ops": actual_warmup_ops,
            "read_ops": args.read_ops,
            "client_num": args.client_num,
            "proxy_num": args.proxy_num,
            "setup_seconds": round(setup_seconds, 3),
            "seconds": round(proxy_perf["seconds"], 3),
            "qps": round(proxy_perf["qps"], 2),
            "latency_ms": {
                key: round(value, 3)
                for key, value in proxy_perf["latency_ms"].items()
            },
            "proxy": round_perf(proxy_perf),
            "work_dir": str(work_dir),
        }
        if direct_perf is not None:
            summary["direct_leader"] = round_perf(direct_perf)
            summary["proxy_to_direct_qps_ratio"] = round(
                proxy_perf["qps"] / direct_perf["qps"], 3
            )
        print(json.dumps(summary, indent=2, sort_keys=True))
        return 0
    finally:
        for driver in proxy_drivers:
            driver.close()
        for driver in direct_drivers:
            driver.close()
        if cluster is not None:
            for driver in cluster["server_drivers"]:
                driver.close()
            for process in reversed(cluster["processes"]):
                process.stop()
        if remove_work_dir:
            shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        sys.exit(130)
