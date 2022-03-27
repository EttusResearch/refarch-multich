"""
Copyright 2019 Ettus Research, A National Instrument Brand
SPDX-License-Identifier: GPL-3.0-or-later
Helper script that provides a Python interface to run the RefArch C++
example.
"""
import argparse
import subprocess
from util import * 

def run(path, params):
    """
    Run benchmark rate and return a CompletedProcess object.
    """
    proc_params = [path]

    for key, val in params.items():
        proc_params.append("--" + str(key))
        proc_params.append(str(val))

    return subprocess.run(proc_params, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def run_cpp_example(build_path: str, exe_name: str, cfgFile: str, params):
    #Call RefArch and save data
    proc_params = ["./"+exe_name, "--cfgFile="+cfgFile]

    for key, val in params.items():
        proc_params.append("--" + str(key))
        proc_params.append(str(val))

    os.chdir(build_path)
    p = subprocess.Popen(proc_params, shell=False)
    out, err = p.communicate()
    errcode = p.returncode
    print(errcode)
    p.kill()
    p.terminate()

def run_batch(build_path, example, iterations, cfgFile, params):
    """
    Runs Arch Example multiple times.
    """
    print("Running Arch Example {} times with the following arguments: ".format(iterations))
    print("cfgFile: " + cfgFile)
    for key, val in params.items():
        print("{:14} {}".format(key, val))

    parsed_results = []
    iteration = 0
    while iteration < iterations:
        proc = run_cpp_example(build_path, example, cfgFile, params)
        iteration += 1

def create_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--args", type=str, help="single uhd device address args")
    parser.add_argument("--time_requested", type=str, help="duration for the test in seconds, set nsamps 0 to use")
    parser.add_argument("--nsamps", type=str, help="number of samples, 0 for continuous/timed")
    return parser
    

def parse_args():
    """
    Parse the command line arguments for Reference Architecture, and returns arguments
    in a dict.
    """
    parser = create_parser()
    params = vars(parser.parse_args())
    return { key : params[key] for key in params if params[key] != None }

def parse_known_args():
    """
    Parse the command line arguments for benchmark rate. Returns a dict
    containing the benchmark rate args, and a list containing args that
    are not recognized by benchmark rate.
    """
    parser = create_parser()
    params, rest = parser.parse_known_args()
    params = vars(params)
    return { key : params[key] for key in params if params[key] != None }, rest


if __name__ == "__main__":
    """
    Main function used for testing only. Requires path to the example to be
    passed as a command line parameter.
    """
    arch_params, rest = parse_known_args()
    parser = argparse.ArgumentParser()
    parser.add_argument("--cfgFile", type=str, required=True,help="path to config file")
    parser.add_argument("--example", type=str, required=True, help="Name of example")
    parser.add_argument("--build-path", type=str, required=True, help="Path to build directory")
    parser.add_argument("--iterations", type=int, required=True, help="Iterations to run example")
    params = parser.parse_args(rest);
    run_batch(params.build_path, params.example,params.iterations, params.cfgFile, arch_params)

   