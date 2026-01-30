<?php
include 'db.php';

$gate_id = 'A1';
$scan_valid = true; // hasil validasi scan

if (!$scan_valid) {
    echo "INVALID";
    exit;
}

$sql = "UPDATE gate_scan
        SET scan_count = scan_count + 1
        WHERE gate_id = $1";

$result = pg_query_params($conn, $sql, [$gate_id]);

if ($result) {
    echo "SCAN_OK";
} else {
    echo "ERROR";
}
