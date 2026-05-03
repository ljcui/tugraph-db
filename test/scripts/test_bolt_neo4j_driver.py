import sys
from concurrent.futures import ThreadPoolExecutor

from neo4j import GraphDatabase, basic_auth
from neo4j.exceptions import Neo4jError
from neo4j.graph import Node, Path, Relationship


DEFAULT_SHARD_KEY = "neo4j-python-sdk"


class TestContext:
    def __init__(self, driver, proxy=False):
        self.driver = driver
        self.proxy = proxy

    def session(self, **kwargs):
        return self.driver.session(database="default", **kwargs)

    def run(self, session, query, **params):
        if self.proxy:
            params.setdefault("_shard_key_", DEFAULT_SHARD_KEY)
        return session.run(query, **params)


def assert_query_works(ctx, expected):
    with ctx.session() as session:
        record = ctx.run(session, "RETURN $value AS n", value=expected).single()
        if record["n"] != expected:
            raise AssertionError(f"unexpected query result: {record['n']}")


def assert_parameter_round_trip(ctx):
    params = {
        "int_value": 42,
        "string_value": "bolt",
        "bool_value": True,
        "float_value": 3.5,
        "list_value": [1, 2, 3],
        "map_value": {"name": "alice", "age": 30},
    }
    with ctx.session() as session:
        record = ctx.run(
            session,
            """
            RETURN $int_value AS int_value,
                   $string_value AS string_value,
                   $bool_value AS bool_value,
                   $float_value AS float_value,
                   $list_value AS list_value,
                   $map_value AS map_value
            """,
            **params,
        ).single()
    for key, value in params.items():
        if record[key] != value:
            raise AssertionError(f"unexpected {key}: {record[key]}")


def assert_fetch_size_streaming(ctx):
    with ctx.session(fetch_size=1) as session:
        result = ctx.run(session, "UNWIND [1, 2, 3, 4] AS n RETURN n")
        values = [record["n"] for record in result]
    if values != [1, 2, 3, 4]:
        raise AssertionError(f"unexpected streamed values: {values}")


def assert_large_result_streaming(ctx):
    expected = list(range(100))
    with ctx.session(fetch_size=7) as session:
        result = ctx.run(session, "UNWIND range(0, 99) AS n RETURN n")
        values = [record["n"] for record in result]
        summary = result.consume()
    if values != expected:
        raise AssertionError(f"unexpected large streamed values: {values}")
    if summary.server is None:
        raise AssertionError("result summary did not include server metadata")


def assert_unconsumed_result_is_recoverable(ctx):
    with ctx.session(fetch_size=1) as session:
        result = ctx.run(session, "UNWIND [1, 2, 3, 4] AS n RETURN n")
        first = next(iter(result))["n"]
        if first != 1:
            raise AssertionError(f"unexpected first streamed value: {first}")

    assert_query_works(ctx, 4)


def assert_overwritten_result_is_recoverable(ctx):
    with ctx.session(fetch_size=1) as session:
        first_result = ctx.run(session, "UNWIND [1, 2, 3, 4] AS n RETURN n")
        first = next(iter(first_result))["n"]
        if first != 1:
            raise AssertionError(f"unexpected first value: {first}")

        second = ctx.run(session, "RETURN 6 AS n").single()["n"]
        if second != 6:
            raise AssertionError(f"unexpected second query value: {second}")


def assert_query_failure_is_recoverable(ctx):
    try:
        with ctx.session() as session:
            ctx.run(session, "THIS IS NOT CYPHER").consume()
    except Neo4jError:
        pass
    else:
        raise AssertionError("invalid query unexpectedly succeeded")

    assert_query_works(ctx, 5)


def assert_repeated_session_reuse(ctx):
    for i in range(10):
        with ctx.session() as session:
            record = ctx.run(session, "RETURN $value AS n", value=i).single()
            if record["n"] != i:
                raise AssertionError(f"unexpected reused session value: {record['n']}")


def assert_concurrent_sessions(ctx):
    def run_query(value):
        with ctx.session(fetch_size=3) as session:
            record = ctx.run(
                session,
                "UNWIND range(0, 9) AS n RETURN sum(n) + $value AS total",
                value=value,
            ).single()
            return record["total"]

    values = list(range(8))
    with ThreadPoolExecutor(max_workers=4) as executor:
        totals = list(executor.map(run_query, values))
    expected = [45 + value for value in values]
    if totals != expected:
        raise AssertionError(f"unexpected concurrent results: {totals}")


def assert_write_read_and_entities(ctx):
    with ctx.session() as session:
        ctx.run(session, "CREATE (:BoltNode {id: 1, name: 'alice'})").consume()
        ctx.run(session, "CREATE (:BoltNode {id: 2, name: 'bob'})").consume()
        ctx.run(
            session,
            """
            MATCH (a:BoltNode {id: 1}), (b:BoltNode {id: 2})
            CREATE (a)-[:BOLT_REL {weight: 7}]->(b)
            """
        ).consume()

        count = ctx.run(session, "MATCH (n:BoltNode) RETURN count(n) AS c").single()[
            "c"
        ]
        if count != 2:
            raise AssertionError(f"unexpected BoltNode count: {count}")

        record = ctx.run(
            session,
            """
            MATCH p=(a:BoltNode {id: 1})-[r:BOLT_REL]->(b:BoltNode {id: 2})
            RETURN a, r, b, p
            """
        ).single()
        if not isinstance(record["a"], Node):
            raise AssertionError(f"unexpected node type: {type(record['a'])}")
        if not isinstance(record["r"], Relationship):
            raise AssertionError(
                f"unexpected relationship type: {type(record['r'])}"
            )
        if not isinstance(record["p"], Path):
            raise AssertionError(f"unexpected path type: {type(record['p'])}")
        if record["a"]["name"] != "alice" or record["b"]["name"] != "bob":
            raise AssertionError("unexpected node properties")
        if record["r"]["weight"] != 7:
            raise AssertionError("unexpected relationship property")


def assert_explicit_transactions_are_recoverable(ctx):
    try:
        with ctx.session() as session:
            session.begin_transaction()
    except Neo4jError as exc:
        text = f"{getattr(exc, 'code', '')} {exc}"
        if "Unimplemented" not in text and "explicit transactions" not in text:
            raise AssertionError(f"unexpected transaction error: {text}")
    else:
        raise AssertionError("explicit transaction unexpectedly succeeded")

    assert_query_works(ctx, 2)


def assert_proxy_requires_shard_key(ctx):
    if not ctx.proxy:
        return
    try:
        with ctx.session() as session:
            session.run("RETURN 1 AS n").consume()
    except Neo4jError as exc:
        text = f"{getattr(exc, 'code', '')} {exc}"
        if "_shard_key_" not in text:
            raise AssertionError(f"unexpected missing shard key error: {text}")
    else:
        raise AssertionError("proxy query without _shard_key_ unexpectedly succeeded")

    assert_query_works(ctx, 8)


def assert_routing_is_rejected_without_breaking_bolt(port, auth):
    routing_driver = GraphDatabase.driver(
        f"neo4j://127.0.0.1:{port}",
        auth=auth,
        encrypted=False,
        connection_timeout=5,
    )
    try:
        try:
            routing_driver.verify_connectivity()
        except Exception:
            pass
        else:
            raise AssertionError("routing unexpectedly succeeded")
    finally:
        routing_driver.close()


def run_common_tests(ctx):
    assert_query_works(ctx, 1)
    assert_parameter_round_trip(ctx)
    assert_fetch_size_streaming(ctx)
    assert_large_result_streaming(ctx)
    assert_unconsumed_result_is_recoverable(ctx)
    assert_overwritten_result_is_recoverable(ctx)
    assert_query_failure_is_recoverable(ctx)
    assert_repeated_session_reuse(ctx)
    assert_concurrent_sessions(ctx)
    assert_write_read_and_entities(ctx)
    assert_explicit_transactions_are_recoverable(ctx)
    assert_proxy_requires_shard_key(ctx)


def open_driver(port, auth, max_connection_lifetime=None):
    kwargs = {
        "auth": auth,
        "encrypted": False,
        "connection_timeout": 5,
    }
    if max_connection_lifetime is not None:
        kwargs["max_connection_lifetime"] = max_connection_lifetime
    return GraphDatabase.driver(f"bolt://127.0.0.1:{port}", **kwargs)


def main():
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        raise SystemExit(
            "usage: test_bolt_neo4j_driver.py <bolt_port> [server|proxy]"
        )
    port = int(sys.argv[1])
    mode = sys.argv[2] if len(sys.argv) == 3 else "server"
    if mode not in ("server", "proxy"):
        raise SystemExit(f"unsupported mode: {mode}")

    auth = basic_auth("admin", "73@TuGraph")
    proxy = mode == "proxy"
    driver = open_driver(port, auth, max_connection_lifetime=60)
    try:
        run_common_tests(TestContext(driver, proxy=proxy))
    finally:
        driver.close()

    assert_routing_is_rejected_without_breaking_bolt(port, auth)

    driver = open_driver(port, auth)
    try:
        assert_query_works(TestContext(driver, proxy=proxy), 3)
    finally:
        driver.close()


if __name__ == "__main__":
    main()
