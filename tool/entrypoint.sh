#!/usr/bin/env bash

# Use LOCAL_USER_ID if passed in at runtime.

if [ -n "${LOCAL_USER_ID}" ]; then
    echo "Starting with UID: $LOCAL_USER_ID"
    usermod -u $LOCAL_USER_ID user
    export HOME=/home/user
    chown -R user:user $HOME

    exec su-exec user "$@"
else
    exec "$@"
fi
