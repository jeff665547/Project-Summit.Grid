import sys
import requests

import os
import os.path as path

import glob as gb
import shutil as sh
import subprocess as sp


if __name__ == "__main__":

    access_token = sys.argv[1]
    
    exe_path  = path.realpath( __file__ )
    proj_dir  = path.realpath( path.join( path.dirname( exe_path ), ".." ))

    build_dir = path.join( proj_dir, "build" )
    bin_path  = path.join( proj_dir, "Summit-Grid/bin/summit-app-grid.exe" )
    #cpack_path = gb.glob( path.join( build_dir, "SummitGrid*.exe" ), recursive=True )[0]

    grid_ver = sp.run([ bin_path, "--version" ], capture_output=True, text=True, shell=True )
    version  = grid_ver.stdout.rstrip()

    print( "\n=== post a tag " + version + " ===\n", flush=True )

    tag_uri  = "http://gitlab.centrilliontech.com.tw:10088/api/v4/projects/147/repository/tags?tag_name=" + version + "&ref=1.3.x"
    tag_head = { "PRIVATE-TOKEN": access_token}

    r = requests.post( tag_uri, headers=tag_head )
    print( r.content )

    print( "\n=== bundle package ===\n", flush=True )

    nsis_tpl_path  = path.join( proj_dir, "bundle\grid-pkg.nsi" )
    nsis_run_path  = path.join( proj_dir, "bundle\grid-pkg-run.nsi" )

    with open( nsis_run_path,"w" ) as nsi_file:
        with open( nsis_tpl_path ) as fp:

            content = fp.read()
            content = content.replace( "<%{}%>".format( "version" ), ( version + "." ))

        nsi_file.write( content )

    child = sp.Popen([ "makensis", nsis_run_path ], cwd=proj_dir, universal_newlines=False, shell=True )
    child.wait()

    if child.returncode != 0: 
        raise RuntimeError( "makensis failed" )

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
    # sh.copyfile( nsis_path, path.join( temp_share, nsis_name ))