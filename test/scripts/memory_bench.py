#!/usr/bin/env python3
"""
TuGraph vector benchmark script over Bolt.

Examples:
    python3 test/scripts/memory_bench.py --mode all --workers 8 --total 100000
    python3 test/scripts/memory_bench.py --mode write --workers 16

Requires:
    pip install neo4j

The script assumes lgraph_server is already running, Bolt is reachable, and
the Memory schema/indexes have already been created.
"""
import argparse
import os
import random
import re
import statistics
import time
import uuid
from concurrent.futures import ProcessPoolExecutor, as_completed
from datetime import datetime, timedelta, timezone
from typing import Any, Dict, List, Optional, Tuple

from neo4j import GraphDatabase, basic_auth


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 7687
DEFAULT_DATABASE = "default"
DEFAULT_USER = "admin"
DEFAULT_PASSWORD = "73@TuGraph"

DEFAULT_LABEL = "Memory"
DEFAULT_USER_ID_INDEX = "bench_memory_user_id"
DEFAULT_DIM = 1024
DEFAULT_USERS = 100
DEFAULT_DOCS_PER_USER = 1000
DEFAULT_AGENT_ID = "bench_presstest_agent"
DEFAULT_USER_PREFIX = "bench_presstest_user"
DEFAULT_IDS_FILE = "/tmp/memory_bench_written_ids.txt"
DEFAULT_CONNECTION_TIMEOUT = 5.0
DEFAULT_SHARD_KEY = "user_id"

VECTOR_PROPERTY = "embedding"

MEMORY_PROPERTY_FIELDS = [
    "id",
    "content",
    "memory_type",
    "user_id",
    "agent_id",
    "run_id",
    "actor_id",
    "role",
    "timestamp",
    "timestamp_num",
    "hash_id",
    "created_at",
    "updated_at",
    "msg_time",
    "msg_year",
    "msg_month",
    "msg_week",
    "msg_weekday",
    "msg_day",
    "msg_time_num",
    "event_time",
    "event_year",
    "event_month",
    "event_week",
    "event_weekday",
    "event_day",
    "event_time_num",
]

SAMPLE_TEXTS = [
    "sunny day walk",
    "hot pot dinner",
    "artificial intelligence changes daily work",
    "vector databases power semantic search",
    "performance testing needs production scale data",
    "distributed systems balance consistency and availability",
    "deep learning training consumes compute resources",
    "agent memory preserves long term context",
]

_IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_process_rng: Optional[random.Random] = None


def _random_seed() -> int:
    return int(time.time() * 1000000000) ^ os.getpid()


def _rng() -> random.Random:
    global _process_rng
    rng = _process_rng
    if rng is None:
        rng = random.Random(_random_seed())
        _process_rng = rng
    return rng


def _validate_identifier(name: str, kind: str) -> None:
    if not _IDENTIFIER_RE.match(name):
        raise ValueError(
            "{} must match {}, got {!r}".format(kind, _IDENTIFIER_RE.pattern, name)
        )


def _bench_user_id(prefix: str, i: int) -> str:
    return "{}_{:03d}".format(prefix, i)


def _rand_vec(dim: int) -> List[float]:
    rng = _rng()
    return [rng.random() for _ in range(dim)]


def _iso_week(dt: datetime) -> int:
    return dt.isocalendar()[1]


def _make_document(user_id: str) -> Dict[str, Any]:
    rng = _rng()
    now = datetime.now(timezone.utc)
    msg_dt = now - timedelta(minutes=rng.randint(0, 10080))
    event_dt = now + timedelta(hours=rng.randint(-48, 48))
    return {
        "id": str(uuid.uuid4()),
        "embedding": _rand_vec(DEFAULT_DIM),
        "content": rng.choice(SAMPLE_TEXTS),
        "memory_type": rng.choice(["episodic", "semantic", "procedural"]),
        "user_id": user_id,
        "agent_id": DEFAULT_AGENT_ID,
        "run_id": "run_{:04d}".format(rng.randint(1, 200)),
        "actor_id": "actor_{:02d}".format(rng.randint(1, 10)),
        "role": rng.choice(["user", "assistant", "system"]),
        "timestamp": now.isoformat(),
        "timestamp_num": int(now.timestamp()),
        "hash_id": uuid.uuid4().hex,
        "created_at": now.isoformat(),
        "updated_at": now.isoformat(),
        "msg_time": msg_dt.isoformat(),
        "msg_year": str(msg_dt.year),
        "msg_month": str(msg_dt.month),
        "msg_week": str(_iso_week(msg_dt)),
        "msg_weekday": str(msg_dt.weekday()),
        "msg_day": str(msg_dt.day),
        "msg_time_num": int(msg_dt.timestamp()),
        "event_time": event_dt.isoformat(),
        "event_year": str(event_dt.year),
        "event_month": str(event_dt.month),
        "event_week": str(_iso_week(event_dt)),
        "event_weekday": str(event_dt.weekday()),
        "event_day": str(event_dt.day),
        "event_time_num": int(event_dt.timestamp()),
    }


def _timed(fn):
    start = time.perf_counter()
    result = fn()
    return result, (time.perf_counter() - start) * 1000


def _pct(sorted_latencies: List[float], p: float) -> float:
    total = len(sorted_latencies)
    if total == 1:
        return sorted_latencies[0]
    idx = p / 100 * (total - 1)
    lo = int(idx)
    hi = min(lo + 1, total - 1)
    return sorted_latencies[lo] + (sorted_latencies[hi] - sorted_latencies[lo]) * (
        idx - lo
    )


def _compute_stats(op: str, latencies: List[float], errors: int, wall: float) -> dict:
    total = len(latencies)
    if total == 0:
        return {"op": op, "count": 0, "errors": errors}

    lat = sorted(latencies)
    return {
        "op": op,
        "count": total,
        "errors": errors,
        "qps": total / wall if wall > 0 else 0,
        "avg_ms": statistics.mean(lat),
        "p50_ms": _pct(lat, 50),
        "p90_ms": _pct(lat, 90),
        "p95_ms": _pct(lat, 95),
        "p99_ms": _pct(lat, 99),
        "min_ms": lat[0],
        "max_ms": lat[-1],
    }


def _print_report(stats_list: List[dict]) -> None:
    headers = [
        "op",
        "count",
        "errors",
        "qps",
        "avg_ms",
        "p50_ms",
        "p90_ms",
        "p95_ms",
        "p99_ms",
        "min_ms",
        "max_ms",
    ]
    rows = []
    for item in stats_list:
        if item["count"] == 0:
            continue
        rows.append(
            [
                item["op"],
                item["count"],
                item["errors"],
                "{:.1f}".format(item["qps"]),
                "{:.1f}".format(item["avg_ms"]),
                "{:.1f}".format(item["p50_ms"]),
                "{:.1f}".format(item["p90_ms"]),
                "{:.1f}".format(item["p95_ms"]),
                "{:.1f}".format(item["p99_ms"]),
                "{:.1f}".format(item["min_ms"]),
                "{:.1f}".format(item["max_ms"]),
            ]
        )
    if not rows:
        print("no successful operations")
        return
    col_widths = [
        max(len(str(h)), max((len(str(row[i])) for row in rows), default=0))
        for i, h in enumerate(headers)
    ]
    fmt = "  ".join("{{:<{}}}".format(width) for width in col_widths)
    print(fmt.format(*headers))
    print("  ".join("-" * width for width in col_widths))
    for row in rows:
        print(fmt.format(*row))


def _split_ranges(total: int, workers: int) -> List[Tuple[int, int]]:
    ranges = []
    for worker_id in range(workers):
        begin = (total * worker_id) // workers
        end = (total * (worker_id + 1)) // workers
        if begin < end:
            ranges.append((begin, end))
    return ranges


def _merge_worker_results(results: List[Tuple[List[float], int, List[str]]]):
    latencies: List[float] = []
    errors = 0
    ids: List[str] = []
    for worker_latencies, worker_errors, worker_ids in results:
        latencies.extend(worker_latencies)
        errors += worker_errors
        ids.extend(worker_ids)
    return latencies, errors, ids


def _run_process_pool(args: argparse.Namespace, total_ops: int, worker, *extra):
    ranges = _split_ranges(total_ops, args.workers)
    with ProcessPoolExecutor(max_workers=len(ranges)) as pool:
        futures = [
            pool.submit(worker, args, begin, end, *extra)
            for begin, end in ranges
        ]
        return [future.result() for future in as_completed(futures)]


class TuGraphBench:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        uri = "bolt://{}:{}".format(args.host, args.port)
        self.driver = GraphDatabase.driver(
            uri,
            auth=basic_auth(DEFAULT_USER, DEFAULT_PASSWORD),
            encrypted=False,
            connection_timeout=DEFAULT_CONNECTION_TIMEOUT,
            max_connection_pool_size=1,
        )
        self._session = self.driver.session(database=self.args.database)
        self.insert_query = self._build_insert_query()
        self.search_query = self._build_search_query()
        self.get_query = self._build_get_query()

    def close(self) -> None:
        self._session.close()
        self.driver.close()

    def session(self):
        return self._session

    def _query_params(self, params: Dict[str, Any]) -> Dict[str, Any]:
        params = dict(params)
        params["_shard_key_"] = params.get(DEFAULT_SHARD_KEY, DEFAULT_SHARD_KEY)
        return params

    def consume(self, query: str, **params: Any) -> None:
        self.session().run(query, **self._query_params(params)).consume()

    def single(self, query: str, **params: Any):
        return self.session().run(query, **self._query_params(params)).single()

    def data(self, query: str, **params: Any) -> List[dict]:
        return self.session().run(query, **self._query_params(params)).data()

    def verify(self) -> None:
        record = self.single("RETURN 1 AS ok")
        if record is None or record["ok"] != 1:
            raise RuntimeError("Bolt connectivity check failed")

    def _build_insert_query(self) -> str:
        fields = MEMORY_PROPERTY_FIELDS + [VECTOR_PROPERTY]
        props = ", ".join("{}: ${}".format(field, field) for field in fields)
        return "CREATE (n:{} {{{}}})".format(DEFAULT_LABEL, props)

    def _build_search_query(self) -> str:
        return (
            "CALL db.index.queryNodes($index_name, $user_id) YIELD node\n"
            "        WITH node AS n, vector.similarity.cosine("
            "node.{vector_property}, ${vector_property}) AS score\n"
            "        RETURN n, score\n"
            "        ORDER BY score DESC\n"
            "        LIMIT 10"
        ).format(
            vector_property=VECTOR_PROPERTY,
        )

    def _build_get_query(self) -> str:
        return "MATCH (n:{label} {{id: $id}}) RETURN n".format(
            label=DEFAULT_LABEL
        )

    def count(self) -> int:
        record = self.single(
            "MATCH (n:{}) RETURN count(n) AS count".format(DEFAULT_LABEL)
        )
        return int(record["count"]) if record is not None else 0


def _do_insert(bench: TuGraphBench, user_id: str) -> Tuple[Optional[float], Optional[str]]:
    doc = _make_document(user_id)
    try:
        _, ms = _timed(lambda: bench.consume(bench.insert_query, **doc))
        return ms, doc["id"]
    except Exception as exc:
        print("[insert] exception: {}".format(exc))
        return None, None


def _do_search_vector(bench: TuGraphBench) -> Optional[float]:
    rng = _rng()
    params = {
        "index_name": DEFAULT_USER_ID_INDEX,
        VECTOR_PROPERTY: _rand_vec(DEFAULT_DIM),
        "user_id": _bench_user_id(
            DEFAULT_USER_PREFIX, rng.randint(1, bench.args.users)
        ),
    }
    try:
        _, ms = _timed(lambda: bench.data(bench.search_query, **params))
        return ms
    except Exception as exc:
        print("[search_vector] exception: {}".format(exc))
        return None


def _do_get(bench: TuGraphBench, node_id: str, user_id: str) -> Optional[float]:
    try:
        record, ms = _timed(
            lambda: bench.single(bench.get_query, id=node_id, user_id=user_id)
        )
        if record is not None:
            return ms
        print("[get] empty result: {}".format(node_id))
    except Exception as exc:
        print("[get] exception: {}".format(exc))
    return None


def _write_process(args: argparse.Namespace, begin: int, end: int):
    bench = TuGraphBench(args)
    latencies: List[float] = []
    errors = 0
    ids: List[str] = []
    try:
        for op in range(begin, end):
            user_idx = op // DEFAULT_DOCS_PER_USER + 1
            ms, node_id = _do_insert(
                bench, _bench_user_id(DEFAULT_USER_PREFIX, user_idx)
            )
            if ms is None:
                errors += 1
            else:
                latencies.append(ms)
                if node_id:
                    ids.append(
                        "{}\t{}".format(
                            node_id, _bench_user_id(DEFAULT_USER_PREFIX, user_idx)
                        )
                    )
    finally:
        bench.close()
    return latencies, errors, ids


def _read_process(args: argparse.Namespace, begin: int, end: int):
    bench = TuGraphBench(args)
    latencies: List[float] = []
    errors = 0
    try:
        for _ in range(begin, end):
            ms = _do_search_vector(bench)
            if ms is None:
                errors += 1
            else:
                latencies.append(ms)
    finally:
        bench.close()
    return latencies, errors, []


def _get_process(
    args: argparse.Namespace,
    begin: int,
    end: int,
    existing_ids: List[Tuple[str, str]],
):
    bench = TuGraphBench(args)
    rng = random.Random(_random_seed())
    latencies: List[float] = []
    errors = 0
    try:
        for _ in range(begin, end):
            node_id, user_id = rng.choice(existing_ids)
            ms = _do_get(bench, node_id, user_id)
            if ms is None:
                errors += 1
            else:
                latencies.append(ms)
    finally:
        bench.close()
    return latencies, errors, []


def bench_write(args: argparse.Namespace) -> None:
    total = args.users * DEFAULT_DOCS_PER_USER
    print("")
    print("=" * 60)
    print("[WRITE] users={} docs_per_user={} total={} processes={}".format(
        args.users, DEFAULT_DOCS_PER_USER, total, args.workers
    ))
    print("=" * 60)

    start = time.perf_counter()
    worker_results = _run_process_pool(args, total, _write_process)
    wall = time.perf_counter() - start

    latencies, errors, written_ids = _merge_worker_results(worker_results)
    print("")
    print("wall_seconds: {:.2f}".format(wall))
    print("")
    _print_report([_compute_stats("insert", latencies, errors, wall)])

    with open(args.ids_file, "w", encoding="utf-8") as fp:
        fp.write("\n".join(written_ids))
    print(
        "[write] saved {} id/user_id pairs to {}".format(
            len(written_ids), args.ids_file
        )
    )


def bench_read(args: argparse.Namespace) -> None:
    print("")
    print("=" * 60)
    print("[READ] total_ops={} processes={}".format(args.total, args.workers))
    print("=" * 60)

    start = time.perf_counter()
    worker_results = _run_process_pool(args, args.total, _read_process)
    wall = time.perf_counter() - start

    latencies, errors, _ = _merge_worker_results(worker_results)
    print("")
    print("wall_seconds: {:.2f}".format(wall))
    print("")
    _print_report([_compute_stats("search_vector", latencies, errors, wall)])


def bench_get(args: argparse.Namespace) -> None:
    print("")
    print("=" * 60)
    print("[GET] total_ops={} processes={}".format(args.total, args.workers))
    print("=" * 60)

    if not os.path.exists(args.ids_file):
        raise FileNotFoundError(
            "{} does not exist; run write first".format(args.ids_file)
        )
    existing_ids: List[Tuple[str, str]] = []
    with open(args.ids_file, "r", encoding="utf-8") as fp:
        for line in fp:
            if not line.strip():
                continue
            parts = line.rstrip("\n").split("\t", 1)
            if len(parts) != 2:
                raise RuntimeError(
                    "{} must contain '<id>\\t<user_id>' lines; run write again".format(
                        args.ids_file
                    )
                )
            existing_ids.append((parts[0], parts[1]))
    if not existing_ids:
        raise RuntimeError("{} is empty; run write first".format(args.ids_file))
    print(
        "[GET] loaded {} id/user_id pairs from {}".format(
            len(existing_ids), args.ids_file
        )
    )

    start = time.perf_counter()
    worker_results = _run_process_pool(args, args.total, _get_process, existing_ids)
    wall = time.perf_counter() - start

    latencies, errors, _ = _merge_worker_results(worker_results)
    print("")
    print("wall_seconds: {:.2f}".format(wall))
    print("")
    _print_report([_compute_stats("get", latencies, errors, wall)])


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="TuGraph vector benchmark over Bolt")
    parser.add_argument(
        "--mode",
        choices=["write", "read", "get", "count", "all"],
        default="all",
        help="benchmark mode",
    )
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--database", default=DEFAULT_DATABASE)
    parser.add_argument("--users", type=int, default=DEFAULT_USERS)
    parser.add_argument("--workers", type=int, default=8, help="client process count")
    parser.add_argument(
        "--total",
        type=int,
        default=100000,
        help="read/get operation count; write ignores this value",
    )
    parser.add_argument("--ids-file", default=DEFAULT_IDS_FILE)
    args = parser.parse_args()

    for name, value in [
        ("label", DEFAULT_LABEL),
        ("user id index", DEFAULT_USER_ID_INDEX),
    ]:
        _validate_identifier(value, name)
    if args.users < 1:
        raise ValueError("--users must be >= 1")
    if args.workers < 1:
        raise ValueError("--workers must be >= 1")
    if args.total < 1 and args.mode in ("read", "get", "all"):
        raise ValueError("--total must be >= 1")
    return args


def main() -> None:
    args = parse_args()
    print(
        "TuGraph vector bench | mode={} host={} port={} database={} "
        "processes={} dim={}".format(
            args.mode,
            args.host,
            args.port,
            args.database,
            args.workers,
            DEFAULT_DIM,
        )
    )

    if args.mode == "count":
        bench = TuGraphBench(args)
        try:
            bench.verify()
            print("[count] {}: {} nodes".format(DEFAULT_LABEL, bench.count()))
        finally:
            bench.close()
        return

    if args.mode in ("write", "all"):
        bench_write(args)

    if args.mode in ("read", "all"):
        bench_read(args)

    if args.mode in ("get", "all"):
        bench_get(args)


if __name__ == "__main__":
    main()
