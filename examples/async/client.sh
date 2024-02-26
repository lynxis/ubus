#!/bin/sh

echo "$(date): echo with empty message"
ubus call async echo
echo "$(date): echo with 'foo' message"
ubus call async echo '{ "message": "foo" }'

echo "$(date): longecho with empty message"
ubus call async longecho
echo "$(date): longecho with 'foo' message"
ubus call async longecho '{ "message": "foo" }'
echo "$(date): done testing"
