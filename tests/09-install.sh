#!/bin/sh

test_description='Test the install command'

WHEREAMI=$(dirname "$(realpath "$0")")
. $WHEREAMI/setup.sh

test_expect_success 'cabin install binary to custom prefix' '
    OUT=$(mktemp -d) &&
    test_when_finished "rm -rf $OUT" &&
    cd $OUT &&
    "$CABIN" new hello_world &&
    cd hello_world &&
    PREFIX_DIR="$OUT/_prefix" &&
    "$CABIN" install --prefix "$PREFIX_DIR" 1>stdout 2>stderr &&
    (
        test -x "$PREFIX_DIR/bin/hello_world"
    ) &&
    (
        TIME=$(cat stderr | grep Installing >/dev/null; echo $?)
    )
'

test_expect_success 'cabin install library to custom libdir' '
    OUT=$(mktemp -d) &&
    test_when_finished "rm -rf $OUT" &&
    cd $OUT &&
    "$CABIN" new --lib mylib >/dev/null &&
    cd mylib &&
    # Provide a minimal lib source to create a library target
    mkdir -p src &&
    cat > src/lib.cc <<-EOF &&
int lib_symbol() { return 0; }
EOF
    "$CABIN" install --prefix "$OUT/_root" --libdir lib64 1>stdout 2>stderr &&
    (
        test -f "$OUT/_root/lib64/libmylib.a"
    )
'

test_done
