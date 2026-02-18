<?php
include 'db.php';

$gate_id = 'A1';

$sql = "SELECT scan_count 
        FROM gate_scan 
        WHERE gate_id = $1";

$result = pg_query_params($conn, $sql, [$gate_id]);

if (!$result) {
    echo "WAIT";
    exit;
}

$row = pg_fetch_assoc($result);

if ($row && intval($row['scan_count']) >= 2) {

    echo "OPEN";

    // reset counter
    $reset = "UPDATE gate_scan 
              SET scan_count = 0 
              WHERE gate_id = $1";
    pg_query_params($conn, $reset, [$gate_id]);

} else {
    echo "WAIT";
}
