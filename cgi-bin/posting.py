#!/usr/bin/env python

import sys

# Required header for CGI script
print("Content-Type: text/html\n")

# Get command line arguments
data = ' '.join(sys.argv[1:]) if len(sys.argv) > 1 else "No data provided"

# HTML content to be printed
print("""
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>CGI Output</title>
</head>
<body>
  <h1>CGI Output</h1>
  <p>Provided argument: </p>
  <p>%s</p>
</body>
</html>
""" % data)

