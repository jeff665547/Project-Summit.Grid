from datetime import datetime
import requests

import sys
import platform

import os
import os.path as path

import glob as gb
import shutil as sh
import subprocess as sp


if __name__ == "__main__":

    now = datetime.now()
    access_token = sys.argv[1]
    time_suffix  = now.strftime( "-%m%d%H%M%W" )
    
    exe_path  = path.realpath( __file__ )
    proj_dir  = path.realpath( path.join( path.dirname( exe_path ), ".." ))

    bundle_dir = path.join( proj_dir, "bundle" )
    stage_dir = path.join( proj_dir, "stage" )
    bin_path  = path.join( stage_dir, "bin\summit-app-grid.exe" )

    grid_ver = sp.run([ bin_path, "--version" ], capture_output=True, text=True, shell=True )
    version  = grid_ver.stdout.rstrip()

    version_time = version + time_suffix

    print( "\n=== post a tag " + version + " ===\n", flush=True )

    tag_uri  = "http://gitlab.centrilliontech.com.tw:10088/api/v4/projects/147/repository/tags?tag_name=" + version + "&ref=1.3.x"
    tag_head = { "PRIVATE-TOKEN": access_token }

    print( "\n" + tag_uri + "\n", flush=True )
    print( "\n" + access_token + "\n", flush=True )

    r = requests.post( tag_uri, headers=tag_head )
    print( r.content )

    if platform.system() == "Windows":

        print( "\n=== bundle package ===\n", flush=True )

        nsis_tpl_path  = path.join( proj_dir, "bundle\grid-pkg.nsi" )

        child = sp.Popen([ "makensis", "/DVERSION=" + version, nsis_tpl_path ], cwd=proj_dir, universal_newlines=False, shell=True )
        child.wait()

        if child.returncode != 0: 
            raise RuntimeError( "makensis failed" )

        print( "\n=== deploy package ===\n", flush=True )

        # temp_share = "\\\\192.168.200.200\\smtdata\\Joye"
        temp_share  = "\\\\192.168.2.21\\temp_share"
        nsis_path   = path.join( bundle_dir, "summit-grid-setup.exe" )

        if not path.isdir( temp_share ):
            print( "connect to " + temp_share, flush=True )

            mount_cmd = "net use /user:{} {} {}".format( "smt", temp_share, "xyz@cen" )
            os.system( mount_cmd )

            if not path.isdir( temp_share ):
                raise RuntimeError( "can't find: {}".format( temp_share ))

        print( "copy to " + temp_share, flush=True )
        sh.copyfile( nsis_path, path.join( proj_dir,   "Summit-Grid_{}.exe".format( version_time )))
        sh.copyfile( nsis_path, path.join( temp_share, "Summit-Grid_{}.exe".format( version_time )))

    else:
        raise RuntimeError( "currently only support windows bundle" )