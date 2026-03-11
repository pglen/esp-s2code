#!/usr/bin/env python

import os, sys, string
from datetime import date

#
# use: conv_http.py in.html out.c varname_for_c
#

ddd = date.today().strftime("%a, %d-%m-%y")

mystr = """
/* =====[ ESP32 project ]=================================================

   File Name:       """  + sys.argv[1] + """

   Description:     Main file

   Revisions:       (for the original file)

      REV   DATE            BY              DESCRIPTION
      ----  -----------     ----------      ------------------------------
      0.00  %s   Peter Glen      Initial version.

   ======================================================================= */

// Page contents as header. DO NOT EDIT. Contents are overwritten at
// compile time.
// Edit the corresponding HTML file instead: """  % ddd + sys.argv[1] + """

const char """ + sys.argv[3] + """ [] =
"""

#print "Doing ", sys.argv[1],  sys.argv[2], sys.argv[3]

newer = False

try:
    #print( sys.argv[1], os.stat(sys.argv[1]).st_mtime,)
    #print( sys.argv[2], os.stat(sys.argv[2]).st_mtime )

    if os.stat(sys.argv[1]).st_mtime >=  os.stat(sys.argv[2]).st_mtime:
        newer = true

except:
    #print( "continue on ", sys.argv[1])
    newer = True

if not newer:
    #print( "Not Newer", sys.argv[1])
    sys.exit(0)

print( "Converting from:", sys.argv[1], "to:",  sys.argv[2])

fpi = open(sys.argv[1], "r")
fpo = open(sys.argv[2], "w")

print( mystr, file=fpo )

for aaa in fpi:
    if aaa != "\n":
        bbb = str.replace(aaa, "\"", '\\"');
        ccc = str.replace(bbb, "\n", "\\n");
        print("\"" + ccc + "\"", file=fpo  )

print(";", file=fpo)

print( "// EOF", file=fpo)

# EOF
