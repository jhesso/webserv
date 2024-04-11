#!/usr/bin/env python3

# Required header for CGI script
print("Content-Type: text/html\n")

# HTML content to be printed
print("""
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>CGI Hello</title>
</head>
<body>
  <h1>I got here from CGI!</h1>
</body>
</html>
""")

