#! /usr/bin/env python3

"""

REVISION HISTORY
 20201111 Split off from q30-get-paths.py

"""

import json

if __name__ == '__main__':
  import sys
  # Read paths from JSON file produced by the previous pass
  paths = json.load(sys.stdin)

  # Print out the paths in normal human-readable form
  for p in paths:
    print(p)
