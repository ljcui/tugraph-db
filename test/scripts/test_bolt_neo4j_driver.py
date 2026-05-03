import sys
from concurrent.futures import ThreadPoolExecutor

from neo4j import GraphDatabase, basic_auth
from neo4j.exceptions import Neo4jError
from neo4j.graph import Node, Path, Relationship


def assert_query_works(driver, expected):
    with driver.session(database="default") as session:
        record = session.run("RETURN $value AS n", value=expected).single()
        if record["n"] != expected:
            raise AssertionError(f"unexpected query result: {record['n']}")


def assert_parameter_round_trip(driver):
    params = {
        "int_value": 42,
        "string_value": "bolt",
        "bool_value": True,
        "float_value": 3.5,
        "list_value": [1, 2, 3],
        "map_value": {"name": "alice", "age": 30},
    }
    with driver.session(database="default") as session:
        record = session.run(
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


def assert_fetch_size_streaming(driver):
    with driver.session(database="default", fetch_size=1) as session:
        result = session.run("UNWIND [1, 2, 3, 4] AS n RETURN n")
        values = [record["n"] for record in result]
    if values != [1, 2, 3, 4]:
        raise AssertionError(f"unexpected streamed values: {values}")


def assert_large_result_streaming(driver):
    expected = list(range(100))
    with driver.session(database="default", fetch_size=7) as session:
        result = session.run("UNWIND range(0, 99) AS n RETURN n")
        values = [record["n"] for record in result]
        summary = result.consume()
    if values != expected:
        raise AssertionError(f"unexpected large streamed values: {values}")
    if summary.server is None:
        raise AssertionError("result summary did not include server metadata")


def assert_unconsumed_result_is_recoverable(driver):
    with driver.session(database="default", fetch_size=1) as session:
        result = session.run("UNWIND [1, 2, 3, 4] AS n RETURN n")
        first = next(iter(result))["n"]
        if first != 1:
            raise AssertionError(f"unexpected first streamed value: {first}")

    assert_query_works(driver, 4)


def assert_overwritten_result_is_recoverable(driver):
    with driver.session(database="default", fetch_size=1) as session:
        first_result = session.run("UNWIND [1, 2, 3, 4] AS n RETURN n")
        first = next(iter(first_result))["n"]
        if first != 1:
            raise AssertionError(f"unexpected first value: {first}")

        second = session.run("RETURN 6 AS n").single()["n"]
        if second != 6:
            raise AssertionError(f"unexpected second query value: {second}")


def assert_query_failure_is_recoverable(driver):
    try:
        with driver.session(database="default") as session:
            session.run("THIS IS NOT CYPHER").consume()
    except Neo4jError:
        pass
    else:
        raise AssertionError("invalid query unexpectedly succeeded")

    assert_query_works(driver, 5)


def assert_repeated_session_reuse(driver):
    for i in range(10):
        with driver.session(database="default") as session:
            record = session.run("RETURN $value AS n", value=i).single()
            if record["n"] != i:
                raise AssertionError(f"unexpected reused session value: {record['n']}")


def assert_concurrent_sessions(driver):
    def run_query(value):
        with driver.session(database="default", fetch_size=3) as session:
            record = session.run(
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


def assert_write_read_and_entities(driver):
    with driver.session(database="default") as session:
        session.run("CREATE (:BoltNode {id: 1, name: 'alice'})").consume()
        session.run("CREATE (:BoltNode {id: 2, name: 'bob'})").consume()
        session.run(
            """
            MATCH (a:BoltNode {id: 1}), (b:BoltNode {id: 2})
            CREATE (a)-[:BOLT_REL {weight: 7}]->(b)
            """
        ).consume()

        count = session.run("MATCH (n:BoltNode) RETURN count(n) AS c").single()["c"]
        if count != 2:
            raise AssertionError(f"unexpected BoltNode count: {count}")

        record = session.run(
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


def assert_explicit_transactions_are_recoverable(driver):
    try:
        with driver.session(database="default") as session:
            session.begin_transaction()
    except Neo4jError as exc:
        text = f"{getattr(exc, 'code', '')} {exc}"
        if "Unimplemented" not in text and "explicit transactions" not in text:
            raise AssertionError(f"unexpected transaction error: {text}")
    else:
        raise AssertionError("explicit transaction unexpectedly succeeded")

    assert_query_works(driver, 2)


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


port = int(sys.argv[1])
auth = basic_auth("admin", "73@TuGraph")
driver = GraphDatabase.driver(
    f"bolt://127.0.0.1:{port}",
    auth=auth,
    encrypted=False,
    connection_timeout=5,
    max_connection_lifetime=60,
)
try:
    assert_query_works(driver, 1)
    assert_parameter_round_trip(driver)
    assert_fetch_size_streaming(driver)
    assert_large_result_streaming(driver)
    assert_unconsumed_result_is_recoverable(driver)
    assert_overwritten_result_is_recoverable(driver)
    assert_query_failure_is_recoverable(driver)
    assert_repeated_session_reuse(driver)
    assert_concurrent_sessions(driver)
    assert_write_read_and_entities(driver)
    assert_explicit_transactions_are_recoverable(driver)
finally:
    driver.close()

assert_routing_is_rejected_without_breaking_bolt(port, auth)

driver = GraphDatabase.driver(
    f"bolt://127.0.0.1:{port}",
    auth=auth,
    encrypted=False,
    connection_timeout=5,
)
try:
    assert_query_works(driver, 3)
finally:
    driver.close()
