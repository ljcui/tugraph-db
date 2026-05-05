#!/usr/bin/env python3
"""
Start a local TuGraph raft cluster for test/scripts/memory_bench.py.

The cluster shape is fixed:
  - 3 lgraph_server processes
  - 1 lgraph_proxy process on bolt://127.0.0.1:7687
  - 16 raft physical graphs: default_s00 ... default_s15
"""
import argparse
import importlib.util
import os
import shutil
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent

BUILD_DIR = REPO_ROOT / "build"
WORK_DIR = Path("/tmp/memory_bench_cluster")

HOST = "127.0.0.1"
AUTH_USER = "admin"
AUTH_PASSWORD = "73@TuGraph"

LOGICAL_GRAPH = "default"
PHYSICAL_GRAPH_PREFIX = "default_s"
PHYSICAL_GRAPH_WIDTH = 2
SHARD_COUNT = 16

PROXY_PORT = 7687
SERVER_BOLT_PORTS = [17687, 17688, 17689]
SERVER_RAFT_PORTS = [18687, 18688, 18689]

SERVER_BOLT_IO_THREADS = 2
SERVER_BOLT_WORKER_THREADS = 8
SERVER_MAX_BOLT_CONNECTIONS = 8000
SERVER_GRAPH_BLOCK_CACHE = 8 * 1024 * 1024 * 1024
SERVER_RAFT_LOG_BLOCK_CACHE = 1024 * 1024 * 1024
SERVER_RAFT_SCHEDULER_SHARDS = 8
SERVER_ASSISTANT_THREADS = 4

PROXY_BOLT_IO_THREADS = 4
PROXY_WORKER_THREADS = 16
PROXY_MAX_CONNECTIONS = 20000
PROXY_BACKEND_MAX_CONNECTIONS_PER_BACKEND = 256
PROXY_BACKEND_BORROW_TIMEOUT_MS = 5000

STARTUP_TIMEOUT_SECONDS = 60.0
LEADER_TIMEOUT_SECONDS = 60.0
INDEX_TIMEOUT_SECONDS = 60.0

LGRAPH_SERVER = BUILD_DIR / "lgraph_server"
LGRAPH_PROXY = BUILD_DIR / "lgraph_proxy"

LABEL = "Memory"
VECTOR_PROPERTY = "embedding"
VECTOR_DIM = 1024

NODE_INDEXES = [
    ("bench_memory_id", ["id"], {"unique": True}),
    ("bench_memory_user_id", ["user_id"], {"unique": False}),
    ("bench_memory_user_timestamp_num", ["user_id", "timestamp_num"], {"unique": False}),
    ("bench_memory_user_msg_time_num", ["user_id", "msg_time_num"], {"unique": False}),
    (
        "bench_memory_user_event_time_num",
        ["user_id", "event_time_num"],
        {"unique": False},
    ),
]
FULLTEXT_INDEX = ("bench_memory_content_ft", [LABEL], ["content"])


def pid_file_for_server(index: int) -> Path:
    return WORK_DIR / "server{}".format(index) / "lgraph_server.pid"


def pid_file_for_proxy() -> Path:
    return WORK_DIR / "proxy" / "lgraph_proxy.pid"


def read_pid(pid_file: Path) -> Optional[int]:
    try:
        value = pid_file.read_text(encoding="utf-8").strip()
    except FileNotFoundError:
        return None
    try:
        pid = int(value)
    except ValueError:
        return None
    return pid if pid > 0 else None


def process_running(pid: int) -> bool:
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def pid_running(pid_file: Path) -> bool:
    pid = read_pid(pid_file)
    return pid is not None and process_running(pid)


def port_open(port: int) -> bool:
    try:
        with socket.create_connection((HOST, port), timeout=0.2):
            return True
    except OSError:
        return False


def wait_for_port(
    port: int,
    name: str,
    proc: Optional[subprocess.Popen],
    timeout: float = STARTUP_TIMEOUT_SECONDS,
) -> None:
    deadline = time.monotonic() + timeout
    last_error: Optional[BaseException] = None
    while time.monotonic() < deadline:
        if proc is not None and proc.poll() is not None:
            raise RuntimeError("{} exited before {}:{} became reachable".format(name, HOST, port))
        try:
            with socket.create_connection((HOST, port), timeout=0.2):
                return
        except OSError as exc:
            last_error = exc
            time.sleep(0.2)
    raise TimeoutError(
        "timed out waiting for {} on {}:{}: {}".format(name, HOST, port, last_error)
    )


def cluster_running() -> bool:
    return any(pid_running(pid_file_for_server(i)) for i in range(1, 4)) or pid_running(
        pid_file_for_proxy()
    )


def require_binaries() -> None:
    for binary in [LGRAPH_SERVER, LGRAPH_PROXY]:
        if not binary.exists() or not os.access(str(binary), os.X_OK):
            raise FileNotFoundError(
                "missing executable: {}\nbuild first: cmake --build build -j28".format(binary)
            )
    if importlib.util.find_spec("neo4j") is None:
        raise RuntimeError(
            "missing Python package: neo4j\ninstall it with: python3 -m pip install neo4j"
        )


def require_ports_free() -> None:
    for port in SERVER_BOLT_PORTS + SERVER_RAFT_PORTS + [PROXY_PORT]:
        if port_open(port):
            raise RuntimeError("port is already in use: {}:{}".format(HOST, port))


def build_proxy_backend_spec() -> str:
    replicas = []
    for index, (bolt_port, raft_port) in enumerate(
        zip(SERVER_BOLT_PORTS, SERVER_RAFT_PORTS), 1
    ):
        replicas.append("{}@{}:{}:{}".format(index, HOST, bolt_port, raft_port))
    return "0-{}={}".format(SHARD_COUNT - 1, ",".join(replicas))


def start_process(
    name: str,
    args: Sequence[str],
    stdout_path: Path,
    pid_file: Path,
) -> subprocess.Popen:
    stdout_path.parent.mkdir(parents=True, exist_ok=True)
    pid_file.parent.mkdir(parents=True, exist_ok=True)
    stdout = stdout_path.open("ab")
    try:
        proc = subprocess.Popen(
            list(args),
            cwd=str(BUILD_DIR),
            stdout=stdout,
            stderr=subprocess.STDOUT,
        )
    finally:
        stdout.close()
    pid_file.write_text("{}\n".format(proc.pid), encoding="utf-8")
    print("started {} pid={}".format(name, proc.pid), flush=True)
    return proc


def start_server(index: int, bolt_port: int, raft_port: int) -> subprocess.Popen:
    server_dir = WORK_DIR / "server{}".format(index)
    server_dir.joinpath("data").mkdir(parents=True, exist_ok=True)
    server_dir.joinpath("log").mkdir(parents=True, exist_ok=True)

    command = [
        str(LGRAPH_SERVER),
        "--mode=run",
        "--host={}".format(HOST),
        "--bolt_port={}".format(bolt_port),
        "--raft_port={}".format(raft_port),
        "--data_path={}".format(server_dir / "data"),
        "--log_path={}".format(server_dir / "log"),
        "--pid_file={}".format(server_dir / "lgraph.pid"),
        "--log_level=info",
        "--bolt_io_thread_num={}".format(SERVER_BOLT_IO_THREADS),
        "--bolt_worker_thread_num={}".format(SERVER_BOLT_WORKER_THREADS),
        "--max_bolt_connections={}".format(SERVER_MAX_BOLT_CONNECTIONS),
        "--graph_block_cache={}".format(SERVER_GRAPH_BLOCK_CACHE),
        "--raft_log_block_cache={}".format(SERVER_RAFT_LOG_BLOCK_CACHE),
        "--raft_scheduler_shards={}".format(SERVER_RAFT_SCHEDULER_SHARDS),
        "--assistant_thread_num={}".format(SERVER_ASSISTANT_THREADS),
    ]
    proc = start_process(
        "lgraph_server {}".format(index),
        command,
        server_dir / "stdout.log",
        pid_file_for_server(index),
    )
    print(
        "  lgraph_server {}: bolt={} raft={}".format(index, bolt_port, raft_port),
        flush=True,
    )
    return proc


def start_proxy() -> subprocess.Popen:
    proxy_dir = WORK_DIR / "proxy"
    proxy_dir.joinpath("log").mkdir(parents=True, exist_ok=True)
    command = [
        str(LGRAPH_PROXY),
        "--proxy_bolt_port={}".format(PROXY_PORT),
        "--proxy_bolt_io_thread_num={}".format(PROXY_BOLT_IO_THREADS),
        "--proxy_worker_thread_num={}".format(PROXY_WORKER_THREADS),
        "--proxy_max_connections={}".format(PROXY_MAX_CONNECTIONS),
        "--proxy_backend_max_connections_per_backend={}".format(
            PROXY_BACKEND_MAX_CONNECTIONS_PER_BACKEND
        ),
        "--proxy_backend_borrow_timeout_ms={}".format(
            PROXY_BACKEND_BORROW_TIMEOUT_MS
        ),
        "--proxy_logical_graph={}".format(LOGICAL_GRAPH),
        "--proxy_physical_graph_prefix={}".format(PHYSICAL_GRAPH_PREFIX),
        "--proxy_shard_count={}".format(SHARD_COUNT),
        "--proxy_shard_id_width={}".format(PHYSICAL_GRAPH_WIDTH),
        "--proxy_raft_backends={}".format(build_proxy_backend_spec()),
        "--log_path={}".format(proxy_dir / "log"),
        "--log_level=info",
    ]
    proc = start_process(
        "lgraph_proxy",
        command,
        proxy_dir / "stdout.log",
        pid_file_for_proxy(),
    )
    print("  lgraph_proxy: bolt={}".format(PROXY_PORT), flush=True)
    return proc


def graph_name(shard_id: int) -> str:
    return "{}{:0{}d}".format(PHYSICAL_GRAPH_PREFIX, shard_id, PHYSICAL_GRAPH_WIDTH)


def ignore_exists(exc: BaseException) -> bool:
    message = str(exc).lower()
    return (
        "already exists" in message
        or "already exist" in message
        or "exists already" in message
    )


def create_raft_graphs_and_indexes() -> None:
    from neo4j import GraphDatabase, basic_auth

    def open_driver(port: int):
        return GraphDatabase.driver(
            "bolt://{}:{}".format(HOST, port),
            auth=basic_auth(AUTH_USER, AUTH_PASSWORD),
            encrypted=False,
            connection_timeout=5,
            max_connection_lifetime=60,
        )

    def consume(driver, database: str, query: str, **params: Any) -> None:
        with driver.session(database=database) as session:
            session.run(query, **params).consume()

    def single(driver, database: str, query: str, key: str, **params: Any):
        with driver.session(database=database) as session:
            record = session.run(query, **params).single()
        if record is None:
            raise RuntimeError("query returned no rows: {}".format(query))
        return record[key]

    def rows(driver, database: str, query: str, **params: Any) -> List[Any]:
        with driver.session(database=database) as session:
            return list(session.run(query, **params))

    def run_idempotent(driver, database: str, query: str, **params: Any) -> None:
        try:
            consume(driver, database, query, **params)
        except Exception as exc:
            if not ignore_exists(exc):
                raise

    def raft_members(graph: str) -> List[Dict[str, Any]]:
        members = []
        for index, (bolt_port, raft_port) in enumerate(
            zip(SERVER_BOLT_PORTS, SERVER_RAFT_PORTS), 1
        ):
            members.append(
                {
                    "node_id": index,
                    "ip": HOST,
                    "bolt_port": bolt_port,
                    "raft_port": raft_port,
                    "graph": graph,
                }
            )
        return members

    def create_raft_graphs(drivers) -> None:
        query = "CALL dbms.graph.createGraphWithRaft($graph_name, $members)"
        for shard_id in range(SHARD_COUNT):
            graph = graph_name(shard_id)
            members = raft_members(graph)
            for driver in drivers:
                run_idempotent(
                    driver,
                    LOGICAL_GRAPH,
                    query,
                    graph_name=graph,
                    members=members,
                )
            print("raft graph ready: {}".format(graph), flush=True)

    def wait_for_leader(drivers, graph: str) -> int:
        query = (
            "CALL dbms.graph.getRaftNodeInfos($graph_name) "
            "YIELD node_id, is_leader "
            "WHERE is_leader = true "
            "RETURN node_id, is_leader"
        )
        deadline = time.monotonic() + LEADER_TIMEOUT_SECONDS
        last_error: Optional[BaseException] = None
        while time.monotonic() < deadline:
            for driver in drivers:
                try:
                    result = rows(driver, LOGICAL_GRAPH, query, graph_name=graph)
                    leaders = [row["node_id"] for row in result]
                    if len(leaders) == 1:
                        return int(leaders[0])
                except Exception as exc:
                    last_error = exc
            time.sleep(0.2)
        raise TimeoutError(
            "timed out waiting for raft leader for {}: {}".format(graph, last_error)
        )

    def show_indexes(driver, graph: str) -> Dict[str, Dict[str, Any]]:
        return {
            row["name"]: dict(row)
            for row in rows(driver, graph, "CALL db.showIndexes()")
        }

    def wait_for_indexes(driver, graph: str, names: List[str]) -> None:
        deadline = time.monotonic() + INDEX_TIMEOUT_SECONDS
        states: Dict[str, str] = {}
        missing = set(names)
        while time.monotonic() < deadline:
            indexes = show_indexes(driver, graph)
            missing = set(names) - set(indexes)
            states = {
                name: indexes[name].get("state")
                for name in names
                if name in indexes
            }
            build_errors = {
                name: indexes[name].get("buildError")
                for name in names
                if name in indexes and indexes[name].get("buildError") is not None
            }
            if build_errors:
                raise RuntimeError(
                    "{} index build errors: {}".format(graph, build_errors)
                )
            if not missing and all(state == "READY" for state in states.values()):
                return
            time.sleep(0.2)
        raise TimeoutError(
            "{} indexes not ready; missing={}, states={}".format(
                graph, sorted(missing), states
            )
        )

    def create_bench_indexes(driver, graph: str) -> None:
        for index_name, properties, config in NODE_INDEXES:
            run_idempotent(
                driver,
                graph,
                "CALL db.index.createNodeIndex($index_name, $label, $properties, $config)",
                index_name=index_name,
                label=LABEL,
                properties=properties,
                config=config,
            )
        run_idempotent(
            driver,
            graph,
            "CALL db.index.vector.createNodeField($label, $property, $config)",
            label=LABEL,
            property=VECTOR_PROPERTY,
            config={"dimension": VECTOR_DIM},
        )
        run_idempotent(
            driver,
            graph,
            "CALL db.index.fulltext.createNodeIndex($index_name, $labels, $properties)",
            index_name=FULLTEXT_INDEX[0],
            labels=FULLTEXT_INDEX[1],
            properties=FULLTEXT_INDEX[2],
        )
        wait_for_indexes(
            driver,
            graph,
            [name for name, _, _ in NODE_INDEXES] + [FULLTEXT_INDEX[0]],
        )

    drivers = [open_driver(port) for port in SERVER_BOLT_PORTS]
    try:
        for driver in drivers:
            ok = single(driver, LOGICAL_GRAPH, "RETURN 1 AS ok", "ok")
            if ok != 1:
                raise RuntimeError("unexpected connectivity result: {}".format(ok))

        create_raft_graphs(drivers)

        leaders: Dict[str, int] = {}
        for shard_id in range(SHARD_COUNT):
            graph = graph_name(shard_id)
            leaders[graph] = wait_for_leader(drivers, graph)
            print(
                "raft leader: {} -> node {}".format(graph, leaders[graph]),
                flush=True,
            )

        for shard_id in range(SHARD_COUNT):
            graph = graph_name(shard_id)
            leader = leaders[graph]
            if leader < 1 or leader > len(drivers):
                raise RuntimeError("invalid leader node {} for {}".format(leader, graph))
            create_bench_indexes(drivers[leader - 1], graph)
            print("bench indexes ready: {}".format(graph), flush=True)
    finally:
        for driver in drivers:
            driver.close()


def start_cluster() -> None:
    if cluster_running():
        print("cluster already appears to be running")
        status_cluster()
        return

    require_binaries()
    require_ports_free()
    WORK_DIR.mkdir(parents=True, exist_ok=True)

    started_any = False
    try:
        server_procs = [
            start_server(1, SERVER_BOLT_PORTS[0], SERVER_RAFT_PORTS[0]),
            start_server(2, SERVER_BOLT_PORTS[1], SERVER_RAFT_PORTS[1]),
            start_server(3, SERVER_BOLT_PORTS[2], SERVER_RAFT_PORTS[2]),
        ]
        started_any = True

        for index, proc in enumerate(server_procs):
            wait_for_port(
                SERVER_BOLT_PORTS[index],
                "lgraph_server {} bolt".format(index + 1),
                proc,
            )
            wait_for_port(
                SERVER_RAFT_PORTS[index],
                "lgraph_server {} raft".format(index + 1),
                proc,
            )

        create_raft_graphs_and_indexes()

        proxy_proc = start_proxy()
        wait_for_port(PROXY_PORT, "lgraph_proxy", proxy_proc)
    except BaseException:
        if started_any:
            print("start failed; stopping partial cluster", file=sys.stderr)
            stop_cluster()
        raise

    print("")
    print("cluster is ready")
    print("proxy: bolt://{}:{}".format(HOST, PROXY_PORT))
    print("work dir: {}".format(WORK_DIR))
    print("")
    print("bench examples:")
    print("  python3 test/scripts/memory_bench.py --mode write --workers 16")
    print("  python3 test/scripts/memory_bench.py --mode read --workers 16")
    print("  python3 test/scripts/memory_bench.py --mode all --workers 16")


def remove_pid_file(pid_file: Path) -> None:
    try:
        pid_file.unlink()
    except FileNotFoundError:
        pass


def wait_for_process_exit(pid: int, timeout: float) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not process_running(pid):
            return True
        time.sleep(0.1)
    return not process_running(pid)


def terminate_process(pid: int, name: str) -> None:
    if not process_running(pid):
        return

    print("stopping {} pid={}".format(name, pid), flush=True)
    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        return

    if wait_for_process_exit(pid, 10.0):
        return

    print("force stopping {} pid={}".format(name, pid), flush=True)
    try:
        os.kill(pid, signal.SIGKILL)
    except ProcessLookupError:
        return
    wait_for_process_exit(pid, 2.0)


def stop_one(pid_file: Path, name: str) -> None:
    pid = read_pid(pid_file)
    if pid is None:
        remove_pid_file(pid_file)
        return
    if not process_running(pid):
        remove_pid_file(pid_file)
        return

    terminate_process(pid, name)
    remove_pid_file(pid_file)


def read_process_cmdline(pid: int) -> List[str]:
    try:
        raw = Path("/proc").joinpath(str(pid), "cmdline").read_bytes()
    except (FileNotFoundError, ProcessLookupError, PermissionError, OSError):
        return []
    return [
        part.decode("utf-8", errors="replace")
        for part in raw.split(b"\0")
        if part
    ]


def is_lgraph_process(cmdline: List[str]) -> bool:
    if not cmdline:
        return False
    executable = Path(cmdline[0]).name
    return executable in {"lgraph_server", "lgraph_proxy"}


def find_lgraph_processes() -> List[Tuple[int, str]]:
    proc = Path("/proc")
    processes: List[Tuple[int, str]] = []
    for child in proc.iterdir():
        if not child.name.isdigit():
            continue
        pid = int(child.name)
        if pid == os.getpid():
            continue
        cmdline = read_process_cmdline(pid)
        if is_lgraph_process(cmdline):
            processes.append((pid, Path(cmdline[0]).name))
    return processes


def stop_remaining_lgraph_processes() -> None:
    processes = find_lgraph_processes()
    if not processes:
        return
    processes.sort(key=lambda item: 0 if item[1] == "lgraph_proxy" else 1)
    for pid, executable in processes:
        terminate_process(pid, "{} remaining".format(executable))


def stop_cluster() -> None:
    stop_one(pid_file_for_proxy(), "lgraph_proxy")
    stop_one(pid_file_for_server(3), "lgraph_server 3")
    stop_one(pid_file_for_server(2), "lgraph_server 2")
    stop_one(pid_file_for_server(1), "lgraph_server 1")
    stop_remaining_lgraph_processes()


def status_one(pid_file: Path, name: str, detail: str) -> None:
    pid = read_pid(pid_file)
    if pid is not None and process_running(pid):
        print("{}: running pid={} {}".format(name, pid, detail))
    else:
        print("{}: stopped {}".format(name, detail))


def status_cluster() -> None:
    status_one(
        pid_file_for_server(1),
        "lgraph_server 1",
        "bolt={} raft={}".format(SERVER_BOLT_PORTS[0], SERVER_RAFT_PORTS[0]),
    )
    status_one(
        pid_file_for_server(2),
        "lgraph_server 2",
        "bolt={} raft={}".format(SERVER_BOLT_PORTS[1], SERVER_RAFT_PORTS[1]),
    )
    status_one(
        pid_file_for_server(3),
        "lgraph_server 3",
        "bolt={} raft={}".format(SERVER_BOLT_PORTS[2], SERVER_RAFT_PORTS[2]),
    )
    status_one(pid_file_for_proxy(), "lgraph_proxy", "bolt={}".format(PROXY_PORT))
    print("work dir: {}".format(WORK_DIR))


def clean_cluster() -> None:
    stop_cluster()
    if str(WORK_DIR) != "/tmp/memory_bench_cluster":
        raise RuntimeError("refusing to remove unexpected work dir: {}".format(WORK_DIR))
    WORK_DIR.mkdir(parents=True, exist_ok=True)
    for child in WORK_DIR.iterdir():
        if child.is_dir() and not child.is_symlink():
            shutil.rmtree(child)
        else:
            child.unlink()
    print("cleaned {}".format(WORK_DIR))


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Start a local TuGraph raft cluster for memory_bench.py.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "After start:\n"
            "  python3 test/scripts/memory_bench.py --mode write --workers 16\n"
            "  python3 test/scripts/memory_bench.py --mode read --workers 16"
        ),
    )
    parser.add_argument(
        "command",
        nargs="?",
        choices=["start", "stop", "restart", "status", "clean"],
        default="start",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    try:
        if args.command == "start":
            start_cluster()
        elif args.command == "stop":
            stop_cluster()
        elif args.command == "restart":
            stop_cluster()
            start_cluster()
        elif args.command == "status":
            status_cluster()
        elif args.command == "clean":
            clean_cluster()
        else:
            raise AssertionError("unhandled command: {}".format(args.command))
    except KeyboardInterrupt:
        return 130
    except Exception as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
