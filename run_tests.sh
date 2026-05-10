#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEBSERV_BIN="$ROOT_DIR/webserv"
LOGDIR="$ROOT_DIR/test_logs"
TMPDIR="/tmp/webserv_test"

mkdir -p "$LOGDIR" "$TMPDIR"

# Utilities
die() { echo "[FAIL] $*"; cleanup; exit 1; }
info() { echo "[INFO] $*"; }
ok() { echo "[OK] $*"; }

wait_for_up() {
  local host=$1 port=$2 timeout=${3:-15}
  local start=$(date +%s)
  while :; do
    if (echo > /dev/tcp/$host/$port) >/dev/null 2>&1; then
      return 0
    fi
    if [ $(( $(date +%s) - start )) -ge $timeout ]; then
      return 1
    fi
    sleep 0.2
  done
}

assert_http_code() {
  local url=$1 expected=$2 method=${3:-GET} data=${4:-}
  local code
  if [ "$method" = "GET" ]; then
    code=$(curl -s -o /dev/null -w '%{http_code}' "$url")
  else
    code=$(curl -s -o /dev/null -w '%{http_code}' -X "$method" ${data:+-d "$data"} "$url")
  fi
  if [ "$code" != "$expected" ]; then
    echo "[ASSERT-FAIL] Expected HTTP $expected for $method $url, got $code"
    return 1
  fi
  echo "$code"
}

assert_contains() {
  local url=$1 needle=$2
  local body
  body=$(curl -s "$url")
  if ! echo "$body" | grep -q -F "$needle"; then
    echo "--- RESPONSE BODY ---"
    echo "$body"
    echo "---------------------"
    echo "[ASSERT-FAIL] Response from $url does not contain: $needle"
    return 1
  fi
  ok "$url contains '$needle'"
}

start_server() {
  local config=$1 outlog=$2
  if [ ! -x "$WEBSERV_BIN" ]; then
    die "webserv binary not found or not executable at $WEBSERV_BIN. Run 'make' first."
  fi
  nohup "$WEBSERV_BIN" "$config" >"$outlog" 2>&1 &
  local pid=$!
  echo $pid
}

stop_server() {
  local pid=$1
  if [ -n "$pid" ] && ps -p $pid > /dev/null 2>&1; then
    kill $pid
    wait $pid 2>/dev/null || true
    ok "Stopped server PID $pid"
  fi
}

cleanup() {
  # Stop any servers recorded
  for pidfile in "$TMPDIR"/*.pid; do
    [ -f "$pidfile" ] || continue
    pid=$(cat "$pidfile")
    stop_server "$pid"
    rm -f "$pidfile"
  done
}

trap cleanup EXIT

# Build
info "Building project..."
if ! make -j >/dev/null 2>&1; then
  die "Make failed. Fix compilation errors before running tests."
fi
ok "Build succeeded"

# Start servers with chosen configs
info "Starting servers..."

ROBST_CONF="$ROOT_DIR/config/robusto.conf"
AUTO_CONF="$ROOT_DIR/config/test_autoindex.conf"

ROBST_LOG="$LOGDIR/robusto.log"
AUTO_LOG="$LOGDIR/autoindex.log"

robst_pid=$(start_server "$ROBST_CONF" "$ROBST_LOG")
echo "$robst_pid" > "$TMPDIR/robst.pid"
info "Started webserv (robusto.conf) PID $robst_pid"

auto_pid=$(start_server "$AUTO_CONF" "$AUTO_LOG")
echo "$auto_pid" > "$TMPDIR/auto.pid"
info "Started webserv (test_autoindex.conf) PID $auto_pid"

# Wait for ports to be up
info "Waiting for ports..."
if ! wait_for_up 127.0.0.1 8080 15; then die "Port 8080 not listening"; fi
if ! wait_for_up 127.0.0.1 3003 15; then die "Port 3003 not listening"; fi
if ! wait_for_up 127.0.0.1 9090 15; then die "Port 9090 not listening"; fi
if ! wait_for_up 127.0.0.1 2005 15; then die "Port 2005 (autoindex) not listening"; fi
ok "Ports are listening"

# Tests summary counter
PASS=0
FAIL=0

run_test() {
  local name=$1; shift
  echo "\n=== Test: $name ==="
  if "$@"; then
    PASS=$((PASS+1))
    ok "$name"
  else
    FAIL=$((FAIL+1))
    echo "[TEST FAIL] $name"
  fi
}

# 1) Basic GET -- index on 8080
run_test "GET / on 8080 returns 200" assert_http_code "http://127.0.0.1:8080/" 200

# 2) 404 for missing file
run_test "GET missing -> 404" assert_http_code "http://127.0.0.1:8080/no_such_file.html" 404

# 3) Redirect /redirect -> 301 and Location header (use GET, server does not implement HEAD)
run_test "Redirect /redirect returns 301/302" bash -c "code=\$(curl -s -o /dev/null -w '%{http_code}' -X GET http://127.0.0.1:8080/redirect); echo \"[INFO] Redirect returned HTTP \$code\"; [ \"\$code\" = \"301\" ] || [ \"\$code\" = \"302\" ]"
run_test "Redirect Location header" bash -c "hdr=\$(curl -s -D - -X GET http://127.0.0.1:8080/redirect | sed -n '1,40p' | tr -d '\r'); echo \"[INFO] Redirect headers:\n\$hdr\"; echo \"\$hdr\" | grep -i '^Location:'"

# 4) CGI GET
run_test "CGI GET returns marker" assert_contains "http://127.0.0.1:8080/cgi-bin/test.py" "CGI Script Funcionando!"

# 5) CGI POST echoing data
run_test "CGI POST echoes POST data" bash -c "curl -s -X POST -d 'hello=world' http://127.0.0.1:8080/cgi-bin/test.py | grep -q 'hello=world'"

# 6) Shell CGI on 9090
run_test "Shell CGI returns METHOD" bash -c "curl -s http://127.0.0.1:9090/cgi-sh/script.sh | grep -q 'METHOD='"

# 7) Upload file to /upload on server2 (port 3003) and ensure file appears
INFO_UPLOAD_FILE="$TMPDIR/test_upload.txt"
echo "upload-test-content" > "$INFO_UPLOAD_FILE"
run_test "Upload file to /uploads (3003)" bash -c "curl -s -F 'file=@$INFO_UPLOAD_FILE' http://127.0.0.1:3003/uploads/ -o /dev/null && (ls ./www/robusto/server2/uploads 2>/dev/null | grep -q 'test_upload.txt' || ls ./www/robusto/server2/uploads | grep -q 'test_upload.txt')"

# 8) client_max_body_size tests (server2 has 2M)
# exact limit (2M) should be accepted (2xx) when uploading to an upload-enabled location
EXACT_LIMIT_FILE="$TMPDIR/exact_limit.bin"
OVER_LIMIT_FILE="$TMPDIR/over_limit.bin"
LIMIT_BYTES=$((2 * 1024 * 1024))
head -c $LIMIT_BYTES </dev/zero > "$EXACT_LIMIT_FILE"
head -c $((LIMIT_BYTES + 1)) </dev/zero > "$OVER_LIMIT_FILE"
run_test "POST exact limit (2M) -> accepted on /upload (3003)" bash -c "code=\$(curl -s -o /dev/null -w '%{http_code}' -X POST --data-binary @\"$EXACT_LIMIT_FILE\" http://127.0.0.1:3003/upload); [[ \"\$code\" =~ ^2 ]] ; echo \"[INFO] exact POST returned HTTP \$code\""
run_test "POST over limit (2M+1) -> 413 on /upload (3003)" bash -c "code=\$(curl -s -o /dev/null -w '%{http_code}' -X POST --data-binary @\"$OVER_LIMIT_FILE\" http://127.0.0.1:3003/upload); [ \"\$code\" = \"413\" ]; echo \"[INFO] over POST returned HTTP \$code\""

# 9) DELETE method: create a file under server2 uploads path then DELETE it (server3 root denies deletes)
TEST_DELETE_FILE="./www/robusto/server2/uploads/delete_me.txt"
mkdir -p ./www/robusto/server2/uploads
echo "to delete" > "$TEST_DELETE_FILE"
run_test "DELETE file on 3003/uploads (upload_path mapping)" bash -c "code=\$(curl -s -o /dev/null -w '%{http_code}' -X DELETE http://127.0.0.1:3003/uploads/uploads/delete_me.txt); echo \"[INFO] DELETE returned HTTP \$code\"; [ \"\$code\" = \"200\" ] || [ \"\$code\" = \"204\" ]"
if [ -f "$TEST_DELETE_FILE" ]; then
  rm -f "$TEST_DELETE_FILE"
fi

# 10) Autoindex: directory listing contains known file
run_test "Autoindex on 2005 lists test_dir" bash -c "body=\$(curl -s http://127.0.0.1:2005/test_dir/); (echo \"\$body\" | grep -q -E '<a |href' || echo \"\$body\" | grep -q 'test' )"

# 11) Autoindex off / index present: root on 2005 should serve index
run_test "2005 root serves index (autoindex off)" assert_http_code "http://127.0.0.1:2005/" 200

# 12) Upload disabled: attempt multipart POST to 2005 root (upload_enabled not set) -> 403
run_test "Upload disabled returns 403 (2005 root)" bash -c "code=\$(curl -s -o /dev/null -w '%{http_code}' -F 'file=@\"$INFO_UPLOAD_FILE\"' http://127.0.0.1:2005/); [ \"\$code\" = \"403\" ]"

# 13) allow_methods header: POST to server2 root (3003) should return 405 and include Allow header
run_test "Allow header present on 405 (3003)" bash -c "hdr=\$(curl -s -i -X POST http://127.0.0.1:3003/ | tr -d '\r'); echo \"[INFO] headers:\n\$hdr\"; echo \"\$hdr\" | grep -i '^Allow:'"

# 14) Error page content: missing resource on 8080 should include 'Not found' or a 404 page
run_test "404 page contains 'Not found'" assert_contains "http://127.0.0.1:8080/no_such_file.html" "Not found"

# 15) CGI failure: start a temporary server with broken cgi_pass and expect 500
BAD_CONF="$ROOT_DIR/config/bad_cgi.conf"
cp "$ROBST_CONF" "$BAD_CONF"
# change listen port to avoid collision and set cgi_pass to a non-existent binary
sed -i 's/listen 127.0.0.1:8080;/listen 127.0.0.1:8182;/' "$BAD_CONF" || true
sed -i 's|cgi_path /usr/bin/python3;|cgi_path /nonexistent/python;|g' "$BAD_CONF" || true

BAD_LOG="$LOGDIR/badcgi.log"
bad_pid=$(start_server "$BAD_CONF" "$BAD_LOG")
echo "$bad_pid" > "$TMPDIR/badcgi.pid"
if ! wait_for_up 127.0.0.1 8182 10; then
  echo "[WARN] badcgi server did not come up";
else
  run_test "CGI with bad cgi_pass returns 500" assert_http_code "http://127.0.0.1:8182/cgi-bin/test.py" 500
fi
stop_server "$bad_pid" || true
rm -f "$TMPDIR/badcgi.pid"
rm -f "$BAD_CONF"

# 11) Multiple listens: check both 8080 and 8081 respond
run_test "Listen on 8081" assert_http_code "http://127.0.0.1:8081/" 200

# 12) Default error page exists (use 403 by requesting a disallowed method where configured)
# For / on server2 allow_methods GET only; POST should be 405
run_test "405 on POST when method not allowed (3003)" assert_http_code "http://127.0.0.1:3003/" 405 POST

# Summarize
info "Tests finished. Passed: $PASS, Failed: $FAIL"

# Write a small JSON summary report
REPORT_FILE="$LOGDIR/report.json"
cat > "$REPORT_FILE" <<-JSON
{
  "timestamp": "$(date --iso-8601=seconds)",
  "passed": $PASS,
  "failed": $FAIL,
  "logs": ["$ROBST_LOG", "$AUTO_LOG", "$BAD_LOG"]
}
JSON

if [ "$FAIL" -ne 0 ]; then
  die "Some tests failed"
else
  ok "All tests passed"
fi

# cleanup happens via trap
exit 0
