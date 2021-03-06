***************************************************************************
(C)MiB <me@not-me.me>

clusterit-mib
=============

Modified version of the ClusterIt-toolset developed by Tim Rightnour

Welcome to "ClusterIt (MiB)" which is based on the great work of
Tim Rightnour (root *at* garbled.net). His sources and documentation are
available at his webpage (http://www.garbled.net/clusterit.html).

I used some of the tools of ClusterIt version 2.5 and soon missed some
features so i modified the tools "dsh", "dvt" and "pcp" as described in this
document.

BE WARNED: i'm not an experienced c-programmer, so my modifications may break
the toolset somehow. The use of these tools may harm your computer's security,
lead to data loss or any kind of unexpected damage. 
USE OF THESE TOOLS IS AT YOUR OWN RISK!

Major changes to the tools are:
* Ability to select the configfile "clusterit" from the working directory or via option "-c <clusterfile>"
* Possibility to have individual portnumbers in the clusterfile (servername:portnumber). 
For a detailed description have a look at the file CHANGES-MiB accompanying this package.

The programs have successfully been compiled and tested on:

* Solaris 10 (SPARC, GCC )
* Solaris 10 (x86, SunStudio 12.3)
* TinyCore Linux 3.0.21 (x86_64, GCC 4.6.1)
* Lubuntu Linux 3.8.0-26 (x86_64, GCC 4.7.3)

(C)MiB <me@not-me.me>
***************************************************************************
