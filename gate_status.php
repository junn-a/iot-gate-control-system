<?php
include 'db.php';

$gate_id = 'A1';

/* OPTIONAL keamanan IP ESP */
// if ($_SERVER['REMOTE_ADDR'] !== 'IP_ESP') exit;

pg_query($conn, "BEGIN");

$sql = "SELECT scan_count
        FROM gate_scan
        WHERE gate_id = $1
        FOR UPDATE";

$result = pg_query_params($conn, $sql, [$gate_id]);
$row = pg_fetch_assoc($result);

if ($row && $row['scan_count'] >= 2) {

    echo "OPEN";

    $reset = "UPDATE gate_scan
              SET scan_count = 0
              WHERE gate_id = $1";
    pg_query_params($conn, $reset, [$gate_id]);

} else {
    echo "WAIT";
}

pg_query($conn, "COMMIT");
