#!/usr/bin/env bash
# Test suite for the todo CLI.
# usage: bash run.sh <path-to-todo-binary> [ITEM ...]     (ITEM = C1..C9, L1..L10, or ALL)
# optional env: TODO_SRC   = extracted source tree (README / contrib checks)
#               TODO_BUILD = cmake build dir (L7 install check)
# The REGRESSION group always runs. Every test group gets a fresh sandbox in a temp dir.
set -u
T=$(cd "$(dirname "$0")" && pwd)
SBXROOT=$(mktemp -d)
trap 'rm -rf "$SBXROOT"' EXIT
if [ $# -lt 1 ] || [ ! -x "$1" ]; then echo "usage: bash run.sh <todo-binary> [ITEM ...]"; exit 2; fi
BIN=$(realpath "$1"); shift
ITEMS=("$@")
TODO_SRC=${TODO_SRC:-}
TODO_BUILD=${TODO_BUILD:-}
ORIGPATH=/usr/local/sbin:/usr/sbin:/usr/bin:/bin
ESC=$(printf '\033')
TAB=$(printf '\t')
TODAY=$(date +%F)
YDAY=$(date -d yesterday +%F)
NPASS=0
NFAIL=0
S=""; DATA=""; GROUP=""
XENV=()

# ---------- helpers ----------
strip() { sed -E "s/${ESC}\[[0-9;?]*[A-Za-z]//g; s/\r//g"; }
oneline() { printf '%s' "$1" | tr '\n' '|' | cut -c1-400; }
pass() { echo "PASS $1"; NPASS=$((NPASS+1)); }
fail() { echo "FAIL $1: $2"; NFAIL=$((NFAIL+1)); }
info() { echo "INFO $1"; }

want() {
    local i
    for i in "${ITEMS[@]}"; do
        [ "$i" = "$1" ] && return 0
        [ "$i" = "ALL" ] && return 0
    done
    return 1
}

new_sbx() {
    GROUP=$1
    S=$SBXROOT/$1
    rm -rf "$S"
    mkdir -p "$S/bin/data" "$S/home" "$S/state" "$S/fake"
    cp "$BIN" "$S/bin/todo"
    DATA=$S/bin/data/tasks.json
    XENV=()
    cat >"$S/fake/notify-send" <<EOF
#!/bin/bash
{ echo "CALL"; for a in "\$@"; do echo "ARG:\$a"; done; } >> "$S/notify.log"
exit \${FAKE_NOTIFY_RC:-0}
EOF
    cat >"$S/fake/fzf" <<EOF
#!/bin/bash
echo "CALL \$*" >> "$S/fzf.log"
exit 130
EOF
    chmod +x "$S/fake/notify-send" "$S/fake/fzf"
}

# run [args...] : stdin is /dev/null unless STDIN_DATA is set (then piped).
# Sets RC, RAWOUT, RAWERR (unstripped), OUT, ERR (ESC-stripped), ALL (OUT+ERR).
run() {
    local o=$S/.out e=$S/.err
    if [ -n "${STDIN_DATA+x}" ]; then
        printf '%s' "$STDIN_DATA" | (cd "$S" && env -u TODO_FILE -u NO_COLOR HOME="$S/home" XDG_STATE_HOME="$S/state" \
            PATH="$S/fake:$S/bin:$ORIGPATH" "${XENV[@]}" timeout 10 "$S/bin/todo" "$@") >"$o" 2>"$e"
        RC=$?
    else
        (cd "$S" && env -u TODO_FILE -u NO_COLOR HOME="$S/home" XDG_STATE_HOME="$S/state" \
            PATH="$S/fake:$S/bin:$ORIGPATH" "${XENV[@]}" timeout 10 "$S/bin/todo" "$@") </dev/null >"$o" 2>"$e"
        RC=$?
    fi
    RAWOUT=$(cat "$o"); RAWERR=$(cat "$e")
    OUT=$(strip <"$o"); ERR=$(strip <"$e")
    ALL="$OUT"$'\n'"$ERR"
}

# ptyrun <mode all|out|err> [args...] : like run but stdout/stderr on a pty. Sets RC, PTYOUT, PTYERR (raw).
ptyrun() {
    local mode=$1; shift
    (cd "$S" && env -u TODO_FILE -u NO_COLOR HOME="$S/home" XDG_STATE_HOME="$S/state" \
        PATH="$S/fake:$S/bin:$ORIGPATH" "${XENV[@]}" python3 -I "$T/ptyrun.py" "$mode" "$S/.pout" "$S/.perr" "$S/bin/todo" "$@") </dev/null
    RC=$?
    PTYOUT=$(cat "$S/.pout"); PTYERR=$(cat "$S/.perr")
}

has()   { if grep -qF -- "$3" <<<"$2"; then pass "$1"; else fail "$1" "expected output containing '$3', got: $(oneline "$2")"; fi; }
hasi()  { if grep -qiF -- "$3" <<<"$2"; then pass "$1"; else fail "$1" "expected output containing '$3' (any case), got: $(oneline "$2")"; fi; }
hasnt() { if grep -qF -- "$3" <<<"$2"; then fail "$1" "expected output NOT containing '$3', got: $(oneline "$2")"; else pass "$1"; fi; }
hasrx() { if grep -qE -- "$3" <<<"$2"; then pass "$1"; else fail "$1" "expected output matching /$3/, got: $(oneline "$2")"; fi; }
eq()    { if [ "$2" = "$3" ]; then pass "$1"; else fail "$1" "expected '$(oneline "$2")', got '$(oneline "$3")'"; fi; }
rc_is() { if [ "$RC" = "$2" ]; then pass "$1"; else fail "$1" "expected exit code $2, got $RC (output: $(oneline "$ALL"))"; fi; }
noesc() { if grep -qF -- "$ESC" <<<"$2"; then fail "$1" "expected no ESC sequences, got: $(oneline "$(printf '%s' "$2" | cat -v)")"; else pass "$1"; fi; }
hasesc() { if grep -qF -- "$ESC" <<<"$2"; then pass "$1"; else fail "$1" "expected ESC colour sequences on a tty, got none: $(oneline "$2")"; fi; }
fexists() { if [ -e "$2" ]; then pass "$1"; else fail "$1" "expected file $2 to exist, it does not"; fi; }
fmissing() { if [ -e "$2" ]; then fail "$1" "expected file $2 NOT to exist, it does"; else pass "$1"; fi; }
count() { grep -cF -- "$2" <<<"$1"; }

jqv() { jq -r "$1" "$DATA" 2>/dev/null; }
field() { jqv ".[] | select(.id==$1) | .$2"; }          # field <id> <name> (null if missing)
ids() { jqv '[.[].id] | sort | map(tostring) | join(" ")'; }
write_json() { mkdir -p "$(dirname "$DATA")"; printf '%s\n' "$1" >"$DATA"; }
jqset() { local tmp=$S/.jq; jq "$1" "$DATA" >"$tmp" && cat "$tmp" >"$DATA"; }
# line number of first line containing text in $OUT (999 if none)
lineof() { local n; n=$(grep -nF -- "$1" <<<"$OUT" | head -1 | cut -d: -f1); echo "${n:-999}"; }
# ids in display order from $OUT (first #N on each task line)
listorder() { grep -oE '^[^#]*#[0-9]+' <<<"$OUT" | grep -oE '#[0-9]+$' | tr '\n' ' ' | sed 's/ $//'; }
help_topic_ok() { # help_topic_ok <name> <topic> <keyword>
    run -h "$2"
    if [ -n "$(tr -d '[:space:]' <<<"$OUT")" ] && ! grep -qF "Unknown help topic" <<<"$ALL" && grep -qiF -- "$3" <<<"$ALL"; then
        pass "$1"
    else
        fail "$1" "expected 'todo -h $2' to print a help section mentioning '$3', got: $(oneline "$ALL")"
    fi
}
readme_has() { # readme_has <name> <text>
    if [ -z "$TODO_SRC" ]; then info "$1 skipped (TODO_SRC not set)"; return; fi
    if grep -qF -- "$2" "$TODO_SRC/README.md"; then pass "$1"; else fail "$1" "expected README.md to contain '$2'"; fi
}

# =====================================================================
g_regression() {
    new_sbx REGRESSION
    run -l
    has REG.empty.list "$ALL" "No tasks yet"
    run -a Low thing
    has REG.add.default.out "$ALL" "Task added: Low thing"
    eq REG.add.default.prio low "$(field 1 priority)"
    eq REG.add.default.desc "Low thing" "$(field 1 description)"
    eq REG.add.default.done false "$(field 1 done)"
    run -a Buy milk high
    has REG.add.high.out "$ALL" "Task added: Buy milk"
    eq REG.add.high.prio high "$(field 2 priority)"
    eq REG.add.high.desc "Buy milk" "$(field 2 description)"
    run -a "Finish project" medium
    eq REG.add.medium.prio medium "$(field 3 priority)"
    eq REG.add.medium.desc "Finish project" "$(field 3 description)"

    run -l
    has REG.list.t1 "$OUT" "#1 Low thing"
    has REG.list.t2 "$OUT" "#2 Buy milk"
    has REG.list.t3 "$OUT" "#3 Finish project"
    has REG.list.todo-tag "$OUT" "[⏳ TODO]"
    has REG.list.summary "$OUT" "Shown: 3 | Done: 0 | Pending: 3"
    local l1 l2 l3; l1=$(lineof "#1 Low"); l2=$(lineof "#2 Buy"); l3=$(lineof "#3 Finish")
    if [ "$l2" -lt "$l3" ] && [ "$l3" -lt "$l1" ]; then pass REG.list.priority-order; else fail REG.list.priority-order "expected high(#2) < medium(#3) < low(#1) line order, got lines $l2,$l3,$l1"; fi
    run -l --high
    has REG.list.high.has "$OUT" "Buy milk"; hasnt REG.list.high.nolow "$OUT" "Low thing"; hasnt REG.list.high.nomed "$OUT" "Finish project"
    run -l --medium
    has REG.list.medium.has "$OUT" "Finish project"; hasnt REG.list.medium.nohigh "$OUT" "Buy milk"
    run -l --low
    has REG.list.low.has "$OUT" "Low thing"; hasnt REG.list.low.nohigh "$OUT" "Buy milk"

    run -d 2
    has REG.done.out "$ALL" "Task 2 marked as done"
    eq REG.done.json true "$(field 2 done)"
    eq REG.done.date "$TODAY" "$(field 2 doneDate)"
    run -l --done
    has REG.list.done.has "$OUT" "#2 Buy milk"; has REG.list.done.tag "$OUT" "[✓ DONE]"; hasnt REG.list.done.nopending "$OUT" "Low thing"
    run -l -d
    has REG.list.done-short "$OUT" "Buy milk"
    run -l --pending
    hasnt REG.list.pending.nodone "$OUT" "Buy milk"; has REG.list.pending.has "$OUT" "Low thing"
    run -l -p --medium
    has REG.list.combo.has "$OUT" "Finish project"; hasnt REG.list.combo.nolow "$OUT" "Low thing"
    run -l
    has REG.list.summary.done "$OUT" "Shown: 3 | Done: 1 | Pending: 2"
    run -d 2
    has REG.undone.out "$ALL" "Task 2 marked as not done"
    eq REG.undone.json false "$(field 2 done)"
    eq REG.undone.date null "$(field 2 doneDate)"
    run -d 99
    has REG.done.unknown "$ALL" "No task with ID 99"
    run -d abc
    has REG.done.nan "$ALL" "Invalid ID"

    run -e 1 high
    has REG.edit.out "$ALL" "✅"
    eq REG.edit.json high "$(field 1 priority)"
    eq REG.edit.desc-kept "Low thing" "$(field 1 description)"
    run -e 1 low
    eq REG.edit.json2 low "$(field 1 priority)"
    run -e 99 high
    has REG.edit.unknown "$ALL" "No task with ID 99"

    run -r 3
    hasrx REG.remove.out "$ALL" "✅.*[Rr]emoved"
    eq REG.remove.json "1 2" "$(ids)"
    run -r 99
    has REG.remove.unknown "$ALL" "No task with ID 99"
    eq REG.remove.unknown.json "1 2" "$(ids)"
    run -r abc
    has REG.remove.nan "$ALL" "Invalid ID"

    # ---- queue ----
    STDIN_DATA=$'Step one\n- Step two\n\n- [ ] Step three\n' run -q Demo medium
    has REG.queue.add "$ALL" 'Queue "Demo" added: 3 steps, current: Step one'
    local q1 q2 q3
    q1=$(jqv '.[] | select(.queue=="Demo" and .step==1) | .id')
    q2=$(jqv '.[] | select(.queue=="Demo" and .step==2) | .id')
    q3=$(jqv '.[] | select(.queue=="Demo" and .step==3) | .id')
    eq REG.queue.step2.desc "Step two" "$(field "${q2:-0}" description)"
    eq REG.queue.step3.marker "Step three" "$(field "${q3:-0}" description)"
    eq REG.queue.step1.visible false "$(field "${q1:-0}" hidden)"
    eq REG.queue.step2.hidden true "$(field "${q2:-0}" hidden)"
    eq REG.queue.prio medium "$(field "${q1:-0}" priority)"
    run -l
    has REG.queue.list.current "$OUT" "[Demo 1/3] Step one"
    hasnt REG.queue.list.hidden "$OUT" "Step two"
    run -l -q
    has REG.queue.list.queued "$OUT" "[Demo 2/3] Step two"
    has REG.queue.list.queued-tag "$OUT" "[🔒 QUEUED]"
    run -l --queued
    has REG.queue.list.queued-long "$OUT" "Step three"
    run -d "${q1:-0}"
    has REG.queue.reveal.out "$ALL" "Next in \"Demo\": #$q2 Step two"
    eq REG.queue.reveal.json false "$(field "${q2:-0}" hidden)"
    run -l
    has REG.queue.reveal.list "$OUT" "[Demo 2/3] Step two"
    run -d "${q2:-0}"
    run -d "${q3:-0}"
    has REG.queue.complete "$ALL" 'Queue "Demo" complete!'
    STDIN_DATA=$'x\n' run -q Demo
    has REG.queue.dup "$ALL" "already exists"
    STDIN_DATA='' run -q Nothing here
    has REG.queue.nosteps "$ALL" "No steps given"
    run -q
    has REG.queue.noname "$ALL" "Usage: todo -q"

    # ---- help / misc ----
    run -h
    has REG.help.main "$OUT" "Commands:"
    has REG.help.main.archive "$OUT" "--archive"
    run -h queue
    has REG.help.queue "$OUT" "QUEUE COMMAND"
    run -h archive
    has REG.help.archive "$OUT" "ARCHIVE COMMAND"
    run -h nosuchtopic
    has REG.help.unknown "$ALL" "Unknown help topic"
    run --bogus
    has REG.unknown.cmd "$ALL" "Unknown or missing command"

    # ---- notify + archive ----
    new_sbx REGRESSION-notify
    run -a Urgent one high          # 1
    run -a Old chore                # 2
    run -a Today chore              # 3
    run -a Ancient chore high       # 4
    run -a Later one medium         # 5
    run -d 2; run -d 3; run -d 4
    jqset '(.[] | select(.id==2 or .id==4) | .doneDate) = "2020-01-01"'
    run -n
    fexists REG.notify.called "$S/notify.log"
    local nlog; nlog=$(cat "$S/notify.log" 2>/dev/null)
    has REG.notify.title "$nlog" "1 high priority task for today"
    has REG.notify.body "$nlog" "#1 Urgent one"
    hasnt REG.notify.nodone "$nlog" "Ancient chore"
    has REG.notify.critical "$nlog" "--urgency=critical"
    eq REG.notify.archived.2 true "$(field 2 archived)"
    eq REG.notify.archived.4 true "$(field 4 archived)"
    eq REG.notify.today-not-archived "false" "$(jqv '.[] | select(.id==3) | (.archived // false)')"
    eq REG.notify.pending-not-archived "false" "$(jqv '.[] | select(.id==1) | (.archived // false)')"
    has REG.notify.stamp "$(cat "$S/state/todo-app/last-notify" 2>/dev/null)" "$TODAY"
    run -l
    hasnt REG.arch.list-hides "$OUT" "Old chore"
    has REG.arch.list-today "$OUT" "Today chore"
    has REG.arch.list-pending "$OUT" "Urgent one"
    run -arch
    has REG.arch.show "$OUT" "#2 Old chore"
    has REG.arch.tag "$OUT" "[📦 ARCHIVED 2020-01-01]"
    hasnt REG.arch.no-pending "$OUT" "Urgent one"
    hasnt REG.arch.no-today "$OUT" "Today chore"
    run -arch --high
    has REG.arch.high.has "$OUT" "Ancient chore"; hasnt REG.arch.high.nolow "$OUT" "Old chore"
    run --archive
    has REG.arch.long "$OUT" "Old chore"
    run -A
    has REG.arch.A "$OUT" "Old chore"
    run -l -arch
    has REG.arch.list-flag "$OUT" "Old chore"
    local c1 c2 c3
    c1=$(grep -c '^CALL' "$S/notify.log" 2>/dev/null)
    run -n
    c2=$(grep -c '^CALL' "$S/notify.log" 2>/dev/null)
    eq REG.notify.once-a-day "$c1" "$c2"
    run -n --force
    c3=$(grep -c '^CALL' "$S/notify.log" 2>/dev/null)
    eq REG.notify.force "$((c1+1))" "$c3"
    run -d 2
    has REG.arch.undone.out "$ALL" "marked as not done"
    eq REG.arch.undone.json "false" "$(jqv '.[] | select(.id==2) | (.archived // false)')"
    run -l
    has REG.arch.undone.list "$OUT" "Old chore"
    run -arch
    hasnt REG.arch.undone.arch "$OUT" "Old chore"
}

# =====================================================================
g_C1() {
    new_sbx C1
    STDIN_DATA=$'Step a\nStep b\nStep c\n' run -q Demo        # 1 2 3
    run -r 1
    rc_is C1.remove-current.rc 0
    eq C1.remove-current.reveal false "$(field 2 hidden)"
    eq C1.remove-current.renum2 1 "$(field 2 step)"
    eq C1.remove-current.renum3 2 "$(field 3 step)"
    run -l
    has C1.remove-current.list "$OUT" "[Demo 1/2] Step b"
    run -l -q
    has C1.remove-current.list-q "$OUT" "[Demo 2/2] Step c"
    hasnt C1.remove-current.no3 "$OUT" "/3]"

    STDIN_DATA=$'x1\nx2\nx3\n' run -q Mid                    # 4 5 6
    run -r 5
    eq C1.remove-hidden.current-visible false "$(field 4 hidden)"
    eq C1.remove-hidden.next-still-hidden true "$(field 6 hidden)"
    eq C1.remove-hidden.renum 2 "$(field 6 step)"
    run -l -q
    has C1.remove-hidden.list1 "$OUT" "[Mid 1/2] x1"
    has C1.remove-hidden.list2 "$OUT" "[Mid 2/2] x3"

    STDIN_DATA=$'y1\ny2\ny3\n' run -q Late                   # 7 8 9
    run -d 7
    run -r 8
    eq C1.after-done.reveal false "$(field 9 hidden)"
    eq C1.after-done.renum-done 1 "$(field 7 step)"
    eq C1.after-done.renum 2 "$(field 9 step)"
    run -l
    has C1.after-done.list "$OUT" "[Late 2/2] y3"
    hasnt C1.after-done.no3of2 "$OUT" "3/2]"
    run -r 7
    run -l
    has C1.remove-done.list "$OUT" "[Late 1/1] y3"
}

g_C2() {
    new_sbx C2
    write_json '{ this is not json'
    cp "$DATA" "$S/corrupt.orig"
    run -l
    rc_is C2.list.rc 1
    has C2.list.stderr "$ERR" "❌ Could not read"
    has C2.list.path "$ERR" "tasks.json"
    hasnt C2.list.nohint "$ALL" "A backup exists"
    if cmp -s "$DATA" "$S/corrupt.orig"; then pass C2.list.untouched; else fail C2.list.untouched "corrupt file was modified: $(oneline "$(head -c 200 "$DATA")")"; fi
    run -a New task high
    rc_is C2.add.rc 1
    has C2.add.stderr "$ERR" "❌ Could not read"
    if cmp -s "$DATA" "$S/corrupt.orig"; then pass C2.add.untouched; else fail C2.add.untouched "corrupt file was overwritten: $(oneline "$(head -c 200 "$DATA")")"; fi
    printf '[]\n' >"$DATA.bak"
    run -l
    rc_is C2.hint.rc 1
    has C2.hint.text "$ERR" "A backup exists: todo -u restores it"
    # truncated JSON (half a file)
    printf '[{"id":1,"description":"x","priority":"low","done":fal' >"$DATA"
    run -d 1
    rc_is C2.truncated.rc 1
    has C2.truncated.msg "$ERR" "❌ Could not read"
}

g_C3() {
    new_sbx C3
    run -a Hello high
    noesc C3.pipe.add "$RAWOUT"
    has C3.pipe.emoji "$RAWOUT" "✅"
    run -a Low one
    run -l
    noesc C3.pipe.list "$RAWOUT"
    has C3.pipe.list.text "$RAWOUT" "#1 Hello"
    run -r 99
    noesc C3.pipe.error.stdout "$RAWOUT"
    noesc C3.pipe.error.stderr "$RAWERR"
    run -h
    noesc C3.pipe.help "$RAWOUT"
    ptyrun all -l
    hasesc C3.tty.colours "$PTYOUT"
    has C3.tty.text "$PTYOUT" "Hello"
    XENV=(NO_COLOR=1)
    ptyrun all -l
    noesc C3.nocolor.tty "$PTYOUT"
    has C3.nocolor.text "$PTYOUT" "Hello"
    has C3.nocolor.emoji "$PTYOUT" "●"
    XENV=(NO_COLOR=)
    ptyrun all -l
    hasesc C3.nocolor-empty.keeps-colours "$PTYOUT"
    XENV=()
    # stdout on a tty, stderr piped / and the other way round
    ptyrun out -l
    hasesc C3.split.stdout-tty "$PTYOUT"
    ptyrun out -r 99
    noesc C3.split.stderr-piped.error "$PTYERR"   # only meaningful once errors go to stderr (L2)
    if want C2; then
        write_json '{ broken'
        ptyrun out -l
        has C3.split.stderr-piped.msg "$PTYERR" "Could not read"
        noesc C3.split.stderr-piped "$PTYERR"
        ptyrun err -l
        has C3.split.stderr-tty.msg "$PTYERR" "Could not read"
        hasesc C3.split.stderr-tty "$PTYERR"
    fi
}

g_C4() {
    new_sbx C4
    run -a Lowly task
    ptyrun all -l
    has C4.gray-90 "$PTYOUT" "${ESC}[90m●"
    hasnt C4.no-38 "$PTYOUT" "${ESC}[38m"
    run -d 1
    jqset '(.[] | .doneDate) = "2020-01-01" | (.[] | .archived) = true'
    ptyrun all -arch
    has C4.gray-archived "$PTYOUT" "${ESC}[90m[📦 ARCHIVED"
}

g_C5() {
    new_sbx C5
    write_json '[
 {"id":5,"description":"five","priority":"low","done":false},
 {"id":3,"description":"three","priority":"low","done":false},
 {"id":9,"description":"nine","priority":"high","done":false},
 {"id":1,"description":"one","priority":"low","done":true},
 {"id":4,"description":"four","priority":"low","done":false},
 {"id":7,"description":"seven","priority":"high","done":false},
 {"id":2,"description":"two","priority":"low","done":false}]'
    run -l
    eq C5.order "#7 #9 #1 #2 #3 #4 #5" "$(listorder)"
    run -l --low
    eq C5.order.low "#1 #2 #3 #4 #5" "$(listorder)"
    run -a newest
    run -l --low
    eq C5.newest-last "#1 #2 #3 #4 #5 #10" "$(listorder)"
}

g_C6() {
    new_sbx C6
    run -l
    run -a Alpha
    fexists C6.bak.created "$DATA.bak"
    cp "$DATA" "$S/snap1"
    run -a Beta
    if cmp -s "$DATA.bak" "$S/snap1"; then pass C6.bak.is-previous; else fail C6.bak.is-previous "expected tasks.json.bak == tasks.json before 'todo -a Beta', got: $(oneline "$(cat "$DATA.bak" 2>/dev/null)")"; fi
    hasnt C6.bak.no-new "$(cat "$DATA.bak" 2>/dev/null)" "Beta"
    has C6.data.has-new "$(cat "$DATA")" "Beta"
    fmissing C6.no-tmp-left "$DATA.tmp"
    local i1 i2; i1=$(stat -c %i "$DATA")
    cp "$DATA" "$S/snap2"
    run -d 1
    i2=$(stat -c %i "$DATA")
    if [ "$i1" != "$i2" ]; then pass C6.rename.new-inode; else fail C6.rename.new-inode "expected tasks.json to be replaced by rename (new inode), inode stayed $i1"; fi
    if cmp -s "$DATA.bak" "$S/snap2"; then pass C6.bak.before-done; else fail C6.bak.before-done "expected .bak == state before 'todo -d 1'"; fi
    cp "$DATA.bak" "$S/snap3"
    run -l
    run -h
    if cmp -s "$DATA.bak" "$S/snap3"; then pass C6.bak.readonly-untouched; else fail C6.bak.readonly-untouched "'todo -l' changed tasks.json.bak"; fi
    eq C6.data.valid "1 2" "$(ids)"
    fmissing C6.no-tmp-left2 "$DATA.tmp"
}

g_C7() {
    new_sbx C7
    run -u
    rc_is C7.nothing.rc 1
    has C7.nothing.msg "$ALL" "❌ Nothing to undo"
    run -a Alpha
    run -a Buy milk
    run -r 2
    eq C7.setup "1" "$(ids)"
    run -u
    rc_is C7.undo-remove.rc 0
    has C7.undo-remove.msg "$ALL" "↩️"
    has C7.undo-remove.word "$ALL" "Undid"
    eq C7.undo-remove.json "1 2" "$(ids)"
    eq C7.undo-remove.desc "Buy milk" "$(field 2 description)"
    if grep -qF "Undid: removed #2 Buy milk" <<<"$ALL"; then info "C7 detailed undo message present"; else info "C7 detailed undo message absent (optional): $(oneline "$ALL")"; fi
    run -l
    has C7.undo-remove.list "$OUT" "Buy milk"
    run -d 1
    eq C7.setup2 true "$(field 1 done)"
    run --undo
    rc_is C7.long.rc 0
    eq C7.long.json false "$(field 1 done)"
    help_topic_ok C7.help.undo undo undo
    help_topic_ok C7.help.u u undo
    run -h
    has C7.help.main "$OUT" "--undo"
    run -h nosuchtopic
    has C7.help.topiclist "$ALL" "undo"
    readme_has C7.readme "todo -u"
}

g_C8() {
    new_sbx C8
    local f=$S/custom/deep/x.json
    XENV=(TODO_FILE="$f")
    run -a Hello there
    rc_is C8.add.rc 0
    fexists C8.file.created "$f"
    eq C8.file.content "Hello there" "$(jq -r '.[0].description' "$f" 2>/dev/null)"
    run -l
    has C8.list.uses-file "$OUT" "Hello there"
    XENV=()
    run -l
    hasnt C8.default.untouched "$OUT" "Hello there"
    hasnt C8.default.file "$(cat "$DATA" 2>/dev/null)" "Hello there"
    # an existing list in HOME is preferred over data/ next to the binary
    local h=$S/home/.local/share/todo-app/tasks.json
    mkdir -p "$(dirname "$h")"
    printf '%s\n' '[{"id":1,"description":"Home list","priority":"low","done":false}]' >"$h"
    printf '[]\n' >"$DATA"
    run -l
    has C8.home.preferred "$OUT" "Home list"
    run -a From new build
    eq C8.home.written "From new build" "$(jq -r '.[] | select(.id==2) | .description' "$h" 2>/dev/null)"
    eq C8.home.bin-data-untouched "[]" "$(tr -d '[:space:]' <"$DATA")"
    XENV=(TODO_FILE="$f")
    run -l
    hasnt C8.home.env-wins "$OUT" "Home list"
    XENV=()
    run -h
    has C8.help.footer "$OUT" "TODO_FILE"
    readme_has C8.readme "TODO_FILE"
}

g_C9() {
    new_sbx C9
    local fixture='[
 {"id":1,"description":"Ancient","priority":"low","done":true,"doneDate":"2020-01-01","archived":true},
 {"id":2,"description":"Recent","priority":"low","done":true,"doneDate":"'"$YDAY"'","archived":true},
 {"id":3,"description":"Undated","priority":"low","done":true,"archived":true},
 {"id":4,"description":"Done not archived","priority":"low","done":true,"doneDate":"2020-01-01"},
 {"id":5,"description":"Pending","priority":"low","done":false}]'
    write_json "$fixture"
    run -arch --clear 30
    rc_is C9.clear.rc 0
    has C9.clear.msg "$ALL" "Removed 2 archived task(s)"
    eq C9.clear.json "2 4 5" "$(ids)"
    run -arch
    has C9.clear.kept "$OUT" "Recent"; hasnt C9.clear.gone "$OUT" "Ancient"
    run -arch --clear 30
    has C9.clear.zero "$ALL" "Removed 0 archived task(s)"
    write_json "$fixture"
    run --archive --clear 30
    rc_is C9.long.rc 0
    eq C9.long.json "2 4 5" "$(ids)"
    write_json "$fixture"
    run -arch --clear
    rc_is C9.missing.rc 1
    has C9.missing.msg "$ALL" "❌ Usage: todo -arch --clear <days>"
    eq C9.missing.json "1 2 3 4 5" "$(ids)"
    run -arch --clear abc
    rc_is C9.invalid.rc 1
    has C9.invalid.msg "$ALL" "❌ Usage: todo -arch --clear <days>"
    eq C9.invalid.json "1 2 3 4 5" "$(ids)"
    run -h archive
    has C9.help "$OUT" "--clear"
}

# =====================================================================
g_L1() {
    new_sbx L1
    run -a high
    rc_is L1.rc 1
    has L1.msg "$ALL" "❌ Description cannot be empty"
    eq L1.nothing-saved "0" "$(jqv 'length')"
    run -a "" low
    rc_is L1.emptystr.rc 1
    eq L1.emptystr.nothing-saved "0" "$(jqv 'length')"
}

g_L2() {
    new_sbx L2
    run --bogus
    rc_is L2.unknown.rc 1
    run -a ok
    rc_is L2.add.rc 0
    run -l
    rc_is L2.list.rc 0
    run -h
    rc_is L2.help.rc 0
    run -r 99
    rc_is L2.remove-unknown.rc 1
    has L2.remove-unknown.stderr "$ERR" "❌"
    hasnt L2.remove-unknown.stdout "$OUT" "❌"
    run -d 99
    rc_is L2.done-unknown.rc 1
    has L2.done-unknown.stderr "$ERR" "❌"
    run -d abc
    rc_is L2.done-nan.rc 1
    has L2.done-nan.stderr "$ERR" "❌"
    hasnt L2.done-nan.stdout "$OUT" "❌"
    run -e 99 high
    rc_is L2.edit-unknown.rc 1
    has L2.edit-unknown.stderr "$ERR" "❌"
    run -e 1
    rc_is L2.edit-usage.rc 1
    has L2.edit-usage.stderr "$ERR" "❌"
    run -q
    rc_is L2.queue-noname.rc 1
    has L2.queue-noname.stderr "$ERR" "❌"
    STDIN_DATA='' run -q Empty
    rc_is L2.queue-nosteps.rc 1
    has L2.queue-nosteps.stderr "$ERR" "❌"
    STDIN_DATA=$'s1\n' run -q Dup
    rc_is L2.queue-ok.rc 0
    STDIN_DATA=$'s1\n' run -q Dup
    rc_is L2.queue-dup.rc 1
    has L2.queue-dup.stderr "$ERR" "❌"
    run -d 1
    rc_is L2.done-ok.rc 0
    run -e 1 high
    rc_is L2.edit-ok.rc 0
    run -a urgent high
    XENV=(FAKE_NOTIFY_RC=1)
    run -n --force
    rc_is L2.notify-fail.rc 1
    has L2.notify-fail.stderr "$ERR" "❌"
    XENV=()
    run -n --force
    rc_is L2.notify-ok.rc 0
    run -r 1
    rc_is L2.remove-ok.rc 0
}

g_L3() {
    new_sbx L3
    run -a call high school
    eq L3.middle.desc "call high school" "$(field 1 description)"
    eq L3.middle.prio low "$(field 1 priority)"
    run -a Finish project high
    eq L3.last.desc "Finish project" "$(field 2 description)"
    eq L3.last.prio high "$(field 2 priority)"
    run -a high Finish
    eq L3.first-not-prio.desc "high Finish" "$(field 3 description)"
    eq L3.first-not-prio.prio low "$(field 3 priority)"
    run -a Buy milk medium
    eq L3.medium.prio medium "$(field 4 priority)"
    STDIN_DATA=$'a\n' run -q My high queue
    eq L3.queue.middle.name "My high queue" "$(jqv '.[] | select(.id==5) | .queue')"
    eq L3.queue.middle.prio low "$(field 5 priority)"
    STDIN_DATA=$'a\n' run -q Ship it medium
    eq L3.queue.last.name "Ship it" "$(jqv '.[] | select(.id==6) | .queue')"
    eq L3.queue.last.prio medium "$(field 6 priority)"
}

g_L4() {
    new_sbx L4
    run -a Old text
    run -e 1 high
    rc_is L4.prio.rc 0
    eq L4.prio.json high "$(field 1 priority)"
    eq L4.prio.desc-kept "Old text" "$(field 1 description)"
    has L4.prio.out "$ALL" "✅"
    run -e 1 "New text"
    rc_is L4.rename.rc 0
    eq L4.rename.desc "New text" "$(field 1 description)"
    eq L4.rename.prio-kept high "$(field 1 priority)"
    has L4.rename.out "$ALL" "✅ Task 1 updated: New text"
    run -e 1 low "Other text"
    eq L4.both-first.desc "Other text" "$(field 1 description)"
    eq L4.both-first.prio low "$(field 1 priority)"
    has L4.both-first.out "$ALL" "Task 1 updated: Other text"
    run -e 1 Final words medium
    eq L4.both-last.desc "Final words" "$(field 1 description)"
    eq L4.both-last.prio medium "$(field 1 priority)"
    run -e 1 call high school
    eq L4.middle.desc "call high school" "$(field 1 description)"
    eq L4.middle.prio medium "$(field 1 priority)"
    run -e 99 "Nope"
    rc_is L4.unknown.rc 1
    has L4.unknown.msg "$ALL" "❌"
    run -e 1
    rc_is L4.usage.rc 1
    eq L4.usage.unchanged "call high school" "$(field 1 description)"
}

g_L5() {
    new_sbx L5
    local i
    for i in 1 2 3 4 5 6 7 8 9 10; do run -a "task$i"; done
    run -d 1 4 7
    rc_is L5.done-list.rc 0
    eq L5.done-list.lines 3 "$(count "$ALL" "marked as done")"
    eq L5.done-list.json "1 4 7" "$(jqv '[.[] | select(.done) | .id] | sort | map(tostring) | join(" ")')"
    run -d 2-3
    rc_is L5.done-range.rc 0
    eq L5.done-range.json "1 2 3 4 7" "$(jqv '[.[] | select(.done) | .id] | sort | map(tostring) | join(" ")')"
    run -r 5 8-9
    rc_is L5.remove-mixed.rc 0
    eq L5.remove-mixed.lines 3 "$(grep -cE '✅.*[Rr]emoved' <<<"$ALL")"
    eq L5.remove-mixed.json "1 2 3 4 6 7 10" "$(ids)"
    run -r 6 99 10
    rc_is L5.remove-partial.rc 1
    has L5.remove-partial.err "$ALL" "❌"
    has L5.remove-partial.err-id "$ALL" "99"
    eq L5.remove-partial.others-applied "1 2 3 4 7" "$(ids)"
    run -d 3 abc
    rc_is L5.done-partial.rc 1
    eq L5.done-partial.applied false "$(field 3 done)"
    STDIN_DATA=$'qa\nqb\nqc\n' run -q Q
    local qa qb qc
    qa=$(jqv '.[] | select(.queue=="Q" and .description=="qa") | .id')
    qb=$(jqv '.[] | select(.queue=="Q" and .description=="qb") | .id')
    qc=$(jqv '.[] | select(.queue=="Q" and .description=="qc") | .id')
    run -d "${qa:-0}" "${qb:-0}"
    rc_is L5.queue.rc 0
    eq L5.queue.done-first true "$(field "${qa:-0}" done)"
    eq L5.queue.done-second true "$(field "${qb:-0}" done)"
    eq L5.queue.reveal-third false "$(field "${qc:-0}" hidden)"
    has L5.queue.next "$ALL" "Next in \"Q\""
    run -l
    has L5.queue.list "$OUT" "qc"
}

g_L6() {
    new_sbx L6
    write_json '[
 {"id":1,"description":"Buy milk","priority":"high","done":false},
 {"id":2,"description":"Milk the cow","priority":"low","done":false},
 {"id":3,"description":"Call mom","priority":"medium","done":false},
 {"id":4,"description":"buy MILK powder","priority":"high","done":true},
 {"id":5,"description":"Old milk carton","priority":"low","done":true,"doneDate":"2020-01-01","archived":true}]'
    run -s milk
    rc_is L6.search.rc 0
    has L6.search.1 "$OUT" "#1 Buy milk"; has L6.search.2 "$OUT" "#2 Milk the cow"; has L6.search.4 "$OUT" "#4 buy MILK powder"
    hasnt L6.search.no3 "$OUT" "Call mom"
    hasnt L6.search.no-archived "$OUT" "Old milk carton"
    has L6.search.format "$OUT" "Shown: 3 | Done: 1 | Pending: 2"
    run --search MILK
    has L6.long.case "$OUT" "Shown: 3"
    run -s buy milk
    has L6.multiword.1 "$OUT" "Buy milk"; has L6.multiword.4 "$OUT" "buy MILK powder"; hasnt L6.multiword.no2 "$OUT" "Milk the cow"
    run -l -p --high -s milk
    has L6.listfilter.has "$OUT" "#1 Buy milk"
    has L6.listfilter.count "$OUT" "Shown: 1"
    run -l --search cow
    has L6.listfilter.long "$OUT" "Milk the cow"; has L6.listfilter.long.count "$OUT" "Shown: 1"
    run -l -s milk --high
    has L6.consumes-rest "$OUT" "No matching tasks"
    run -s zzz
    rc_is L6.nomatch.rc 0
    has L6.nomatch.msg "$OUT" "No matching tasks"
    run -arch -s milk
    has L6.arch.has "$OUT" "Old milk carton"; hasnt L6.arch.no-normal "$OUT" "Buy milk"
    run -l -arch -s carton
    has L6.arch.listflag "$OUT" "Old milk carton"
    help_topic_ok L6.help.topic search search
    run -h
    has L6.help.main "$OUT" "--search"
    run -h nosuchtopic
    has L6.help.topiclist "$ALL" "search"
    readme_has L6.readme "todo -s"
}

g_L7() {
    new_sbx L7
    if [ -z "$TODO_SRC" ]; then fail L7.src "TODO_SRC not set, cannot find contrib/"; return; fi
    local comp=$TODO_SRC/contrib/todo-completion.bash zcomp=$TODO_SRC/contrib/_todo
    fexists L7.bash.file "$comp"
    fexists L7.zsh.file "$zcomp"
    has L7.zsh.compdef "$(head -3 "$zcomp" 2>/dev/null)" "#compdef todo"
    run -a Buy milk high
    run -a Call mom
    local script='
        [ -f /usr/share/bash-completion/bash_completion ] && . /usr/share/bash-completion/bash_completion >/dev/null 2>&1
        . "$1" || { echo "SOURCE-FAILED"; exit 1; }
        shift
        spec=$(complete -p todo 2>/dev/null) || { echo "NO-COMPLETE-SPEC"; exit 1; }
        func=${spec##*-F }; func=${func%% *}
        COMP_WORDS=("$@"); COMP_CWORD=$(( $# - 1 ))
        COMP_LINE="$*"; COMP_POINT=${#COMP_LINE}
        COMPREPLY=()
        prev=""; [ $COMP_CWORD -gt 0 ] && prev=${COMP_WORDS[COMP_CWORD-1]}
        "$func" todo "${COMP_WORDS[COMP_CWORD]}" "$prev"
        printf "%s\n" "${COMPREPLY[@]}"'
    comp_run() { (cd "$S" && env -u TODO_FILE HOME="$S/home" XDG_STATE_HOME="$S/state" PATH="$S/fake:$S/bin:$ORIGPATH" \
        timeout 10 bash -c "$script" _ "$comp" "$@" 2>&1) ; }
    local r
    r=$(comp_run todo -)
    hasrx L7.flags.-a "$r" '^-a$'; hasrx L7.flags.--add "$r" '^--add$'; hasrx L7.flags.-l "$r" '^-l$'; hasrx L7.flags.-h "$r" '^-h$'
    hasrx L7.flags.--queue "$r" '^--queue$'
    r=$(comp_run todo --a)
    hasrx L7.prefix.--add "$r" '^--add$'; if grep -qE '^--list$' <<<"$r"; then fail L7.prefix.filtered "'todo --a<TAB>' should not offer --list, got: $(oneline "$r")"; else pass L7.prefix.filtered; fi
    r=$(comp_run todo -h "")
    hasrx L7.help.add "$r" '^add$'; hasrx L7.help.list "$r" '^list$'; hasrx L7.help.queue "$r" '^queue$'; hasrx L7.help.archive "$r" '^archive$'
    if ! want L10; then info "L7 task-ID completion checks deferred until L10 (todo --ids) is logged"; else
    r=$(comp_run todo -d "")
    hasrx L7.ids.-d.1 "$r" '^1([^0-9]|$)'; hasrx L7.ids.-d.2 "$r" '^2([^0-9]|$)'
    has L7.ids.-d.desc "$r" "Buy milk"
    r=$(comp_run todo -r "")
    hasrx L7.ids.-r "$r" '^1([^0-9]|$)'
    r=$(comp_run todo -e "")
    hasrx L7.ids.-e "$r" '^2([^0-9]|$)'
    r=$(comp_run todo -i "")
    hasrx L7.ids.-i "$r" '^1([^0-9]|$)'
    r=$(comp_run todo -d 2)
    if [ -n "$r" ] && ! grep -qvE '^2' <<<"$r"; then pass L7.ids.prefix; else fail L7.ids.prefix "'todo -d 2<TAB>' should offer only task 2, got: $(oneline "$r")"; fi
    fi
    if [ -n "$TODO_BUILD" ]; then
        if cmake --install "$TODO_BUILD" --prefix "$S/inst" >"$S/install.log" 2>&1; then pass L7.install.runs; else fail L7.install.runs "cmake --install failed: $(oneline "$(tail -5 "$S/install.log")")"; fi
        fexists L7.install.bash "$S/inst/share/bash-completion/completions/todo"
        fexists L7.install.zsh "$S/inst/share/zsh/site-functions/_todo"
    else
        info "L7 install check skipped (TODO_BUILD not set)"
    fi
    readme_has L7.readme "Shell completion"
}

g_L8() {
    new_sbx L8
    write_json '[
 {"id":1,"description":"H1","priority":"high","done":false},
 {"id":2,"description":"H2","priority":"high","done":false},
 {"id":3,"description":"H3","priority":"high","done":true,"doneDate":"'"$TODAY"'"},
 {"id":4,"description":"M1","priority":"medium","done":false},
 {"id":5,"description":"L1","priority":"low","done":false},
 {"id":6,"description":"L2","priority":"low","done":false},
 {"id":7,"description":"L3","priority":"low","done":false},
 {"id":8,"description":"Q1","priority":"high","done":false,"queue":"Q","step":1,"hidden":false},
 {"id":9,"description":"Q2","priority":"high","done":false,"queue":"Q","step":2,"hidden":true},
 {"id":10,"description":"A1","priority":"high","done":true,"doneDate":"2020-01-01","archived":true}]'
    run -c
    rc_is L8.rc 0
    eq L8.value "🔥3 ⏳7" "$RAWOUT"
    local nl; nl=$(wc -l <"$S/.out" | tr -d ' ')
    if [ "$nl" -le 1 ] && ! grep -q '^$' "$S/.out"; then pass L8.one-line; else fail L8.one-line "expected a single line without blank padding, got $nl newline(s): $(oneline "$(cat -A "$S/.out")")"; fi
    noesc L8.no-colour "$RAWOUT"
    run --count
    eq L8.long "🔥3 ⏳7" "$RAWOUT"
    write_json '[{"id":1,"description":"x","priority":"high","done":true,"doneDate":"'"$TODAY"'"}]'
    run -c
    eq L8.zero "✅0" "$RAWOUT"
    write_json '[]'
    run -c
    eq L8.zero-empty "✅0" "$RAWOUT"
    help_topic_ok L8.help.topic count count
    run -h
    has L8.help.main "$OUT" "--count"
    run -h nosuchtopic
    has L8.help.topiclist "$ALL" "count"
    readme_has L8.readme "PS1"
}

g_L9() {
    new_sbx L9
    run -a First task
    run -d
    rc_is L9.done.rc 1
    has L9.done.msg "$ALL" "❌ Usage: todo -d <id>"
    eq L9.done.unchanged false "$(field 1 done)"
    run -r
    rc_is L9.remove.rc 1
    has L9.remove.msg "$ALL" "❌ Usage: todo -r <id>"
    eq L9.remove.unchanged "1" "$(ids)"
    if want L10; then
        run -i
        rc_is L9.info.rc 1
        has L9.info.msg "$ALL" "❌ Usage: todo -i <id>"
    else
        info "L9 -i checks deferred until L10 is logged"
    fi
    STDIN_DATA=$'1\n' run -d
    rc_is L9.pipe.rc 1
    eq L9.pipe.unchanged false "$(field 1 done)"
    fmissing L9.no-fzf "$S/fzf.log"
}

g_L10() {
    new_sbx L10
    write_json '[
 {"id":1,"description":"Buy milk","priority":"high","done":false},
 {"id":2,"description":"Old thing","priority":"low","done":true,"doneDate":"2026-10-30"},
 {"id":3,"description":"Pack","priority":"medium","done":false,"queue":"Trip","step":1,"hidden":false},
 {"id":4,"description":"Drive","priority":"medium","done":false,"queue":"Trip","step":2,"hidden":true},
 {"id":5,"description":"Arrive","priority":"medium","done":false,"queue":"Trip","step":3,"hidden":true},
 {"id":6,"description":"Ancient","priority":"low","done":true,"doneDate":"2020-01-01","archived":true}]'
    run -i 1
    rc_is L10.pending.rc 0
    has L10.pending.id "$OUT" "1"; has L10.pending.desc "$OUT" "Buy milk"; hasi L10.pending.prio "$OUT" "high"; has L10.pending.status "$OUT" "TODO"
    run --info 2
    rc_is L10.done.rc 0
    has L10.done.status "$OUT" "DONE"; has L10.done.date "$OUT" "2026-10-30"; has L10.done.desc "$OUT" "Old thing"
    run -i 4
    has L10.queued.status "$OUT" "QUEUED"; has L10.queued.name "$OUT" "Trip"; has L10.queued.step "$OUT" "2/3"
    run -i 3
    has L10.queue-current.name "$OUT" "Trip"; has L10.queue-current.step "$OUT" "1/3"
    run -i 6
    has L10.archived.status "$OUT" "ARCHIVED"; has L10.archived.date "$OUT" "2020-01-01"
    run -i 99
    rc_is L10.unknown.rc 1
    has L10.unknown.msg "$ALL" "❌"
    run -i abc
    rc_is L10.nan.rc 1
    run --ids
    rc_is L10.ids.rc 0
    has L10.ids.1 "$RAWOUT" "1${TAB}Buy milk"
    has L10.ids.queue-current "$RAWOUT" "3${TAB}Pack"
    hasnt L10.ids.no-hidden "$RAWOUT" "Drive"
    hasnt L10.ids.no-archived "$RAWOUT" "Ancient"
    noesc L10.ids.no-colour "$RAWOUT"
    if grep -qvE "^[0-9]+${TAB}" <<<"$RAWOUT"; then fail L10.ids.format "every line must be id<TAB>description, got: $(oneline "$(cat -A <<<"$RAWOUT")")"; else pass L10.ids.format; fi
    run -h
    hasnt L10.ids.hidden-from-help "$OUT" "--ids"
    has L10.help.main "$OUT" "--info"
    help_topic_ok L10.help.topic info info
    run -h nosuchtopic
    has L10.help.topiclist "$ALL" "info"
    readme_has L10.readme "todo -i"
}

# =====================================================================
echo "# binary: $BIN"
echo "# items: ${ITEMS[*]:-<none>}"
g_regression
for it in C1 C2 C3 C4 C5 C6 C7 C8 C9 L1 L2 L3 L4 L5 L6 L7 L8 L9 L10; do
    if want "$it"; then "g_$it"; fi
done
echo "# TOTAL: $NPASS passed, $NFAIL failed"
[ "$NFAIL" -eq 0 ]
