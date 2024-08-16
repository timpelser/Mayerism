import os
import argparse
import inspect

parser = argparse.ArgumentParser(description='')
parser.add_argument('--dryrun', '-d', dest='dryrun', action='store_true', help='Dry run.')
parser.add_argument('--native', '-n', dest='native', action='store_true', help='Use native CPU Architecture Optimizations.')
parser.add_argument('--prefix', '-p', type=str, help='Prebuilt JUCE library path (Optional).')

args = parser.parse_args()

script_root_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
repo_dir = os.path.abspath(os.path.join(script_root_dir, os.pardir))

if not args.dryrun:
    if not os.name == 'nt':
        print("Script only usable in Windows!\nExiting...")
        exit()

os.system('cls' if os.name == 'nt' else 'clear')

if not os.path.exists(f'{repo_dir}/sdk'):
    print(f'Creating {repo_dir}\\sdk..\n')
    if not args.dryrun:
        os.system(f'mkdir {repo_dir}\\sdk')

if not os.path.exists(f'{repo_dir}/sdk/asiosdk_2.3.3_2019-06-14'):

    # Download ASIO SDK
    print("Downloading ASIO SDK...\n")
    
    dl_url = 'https://www.steinberg.net/asiosdk'
    unzip = f'powershell Expand-Archive -Force {repo_dir}\\sdk\\asio.zip -DestinationPath {repo_dir}\\sdk\\' 
    dl_command = f'curl -L -o {repo_dir}\\sdk\\asio.zip {dl_url} && {unzip} && del {repo_dir}\\sdk\\asio.zip'

    if args.dryrun:
        print(f'{dl_command}\n')
    else:
        os.system(dl_command)


cmake_command = f'cmake -S {repo_dir} -B {repo_dir}\\build'

if args.prefix != None:
    cmake_command += f' -DCMAKE_PREFIX_PATH={args.prefix}'

if args.native:
    cmake_command += ' -DUSE_NATIVE_ARCH=1'

cmake_command += f' -DASIO_PATH={repo_dir}\\sdk\\asiosdk_2.3.3_2019-06-14\\common'
cmake_command += f' && cmake --build {repo_dir}\\build --config Release -j %NUMBER_OF_PROCESSORS%'

print("Building project...\n")
print(f'{cmake_command}\n')

if not args.dryrun:
    os.system(cmake_command)
