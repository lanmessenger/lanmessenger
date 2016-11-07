 
#!/bin/sh
 appname=lmc

 dirname=/usr/lib/lmc
 tmp="${dirname#?}"

 if [ "${dirname%$tmp}" != "/" ]; then
 dirname=$PWD/$dirname
 fi
 LD_LIBRARY_PATH=$dirname
 export LD_LIBRARY_PATH
 $dirname/$appname $*