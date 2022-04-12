from datetime import datetime
import requests

import sys
import platform

import os
import os.path as path

import glob as gb
import shutil as sh

if __name__ == "__main__":

    now     = datetime.now()
    
    exe_path  = path.realpath( __file__ )
    proj_dir  = path.realpath( path.dirname( exe_path ))

    build_dir = path.join( proj_dir, "build" )
    nsis_path = gb.glob( path.join( build_dir, "SummitGrid*.exe" ), recursive=True )[0]

    nsis_name = path.basename( nsis_path )
    version = nsis_name.split( "_" )[-1][:-4]

    access_token = sys.argv[1]
    time_suffix  = now.strftime( "-%m%d%H%M%W" )
    version_time = version + time_suffix

    print( "\n=== post a tag " + version_time + " ===\n", flush=True )

    tag_uri  = "http://gitlab.centrilliontech.com.tw:10088/api/v4/projects/147/repository/tags?tag_name=" + version_time + "&ref=1.3.x"
    tag_head = { "PRIVATE-TOKEN": access_token}

    r = requests.post( tag_uri, headers=tag_head )
    print( r.content )

    if platform.system() == "Windows":

        print( "\n=== deploy package ===\n", flush=True )

        # temp_share = "\\\\192.168.200.200\\smtdata\\Joye"
        temp_share = "\\\\192.168.2.21\\temp_share"
        
        if not path.isdir( temp_share ):
            print( "connect to " + temp_share, flush=True )

            mount_cmd = "net use /user:{} {} {}".format( "smt", temp_share, "xyz@cen" )
            os.system( mount_cmd )

            if not path.isdir( temp_share ):
                raise RuntimeError( "can't find: {}".format( temp_share ))

        print( "copy to " + temp_share, flush=True )
        sh.copyfile( nsis_path, path.join( temp_share, nsis_name ))

    else:
        raise RuntimeError( "currently only support windows bundle" )
