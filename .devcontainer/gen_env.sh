#!/usr/bin/env bash

set -ue
set -o pipefail

if [[ $(echo 'x\ty') = 'x\ty' ]]; then
  echo -e "HOST_OS=Linux\nUSERNAME=$USER\nUSER_UID=$(id -u $USER)\nGROUPNAME=$(id -gn $USER)\nGROUP_GID=$(id -g $USER)" > .devcontainer/.env;
else
  echo "HOST_OS=Darwin\nUSERNAME=$USER\nUSER_UID=$(id -u $USER)\nGROUPNAME=root\nGROUP_GID=0" > .devcontainer/.env;  
fi
