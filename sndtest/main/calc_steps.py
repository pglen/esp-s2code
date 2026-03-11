# Calculate reverse power of sample size

import os, sys, getopt

# Configure generated parameters here
startx = 98.;
maxlen = 96
#coeff = startx / maxlen
coeff = 1

def helpx():
    pass

def main():

    vals = [] ; opts = [] ; args = []
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hvVfe:p:s:x:")
    except getopt.GetoptError as err:
        print("Invalid option(s) on command line:", err)
        sys.exit(1)
    _ = args; _ = opts  # silence warnings
    #print "opts", opts, "args", args
    for aa in opts:
        if aa[0] == "-h": helpx()

    global startx
    for aa in range(maxlen):
        vals.append(int(startx))
        startx -= coeff

    #print("//vals", vals)
    #for cnt, aa in enumerate(vals):
    #    print("int val%s = %d;" % (hex(cnt+1)[2:], vals[cnt]))

    OUSTR = "OUTP_SIZE"

    print("#define %s %d"  % (OUSTR, len(vals)) )
    #print("int filt[%s] = {0, };"  %  OUSTR)
    #print("int filt2[%s] = {0, };" %  OUSTR)
    print("int res[%s]  = {0, };"  %  OUSTR)
    print("int res2[%s]  = {0, };" %  OUSTR)
    print("int valarr[%s] = {" % OUSTR, end = " ")

    for cnt, aa in enumerate(vals):
        print("%d, " % vals[cnt], end = " ")
        if cnt % 9 == 8:
            print("\n                 ", end = " ")
    print("};");

if __name__ == '__main__':
    main()

# EOF
