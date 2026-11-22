#!/bin/bash
ports=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
exec /bin/bash "$ports/dynabrocollection/launch.sh" 1 "$@"
