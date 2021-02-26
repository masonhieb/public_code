# BSD 3-Clause License

# Copyright (c) 2021, Mason Hieb
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# 1. Redistributions of source code must retain the above copyright notice, this
   # list of conditions and the following disclaimer.

# 2. Redistributions in binary form must reproduce the above copyright notice,
   # this list of conditions and the following disclaimer in the documentation
   # and/or other materials provided with the distribution.

# 3. Neither the name of the copyright holder nor the names of its
   # contributors may be used to endorse or promote products derived from
   # this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Script that takes a list of directories and file types, and then generates a .bat file that copies all files from those directories into the destination directory using xcopy
# I use this for quickly transfering C++ projects between systems locally without using git

import pathlib
import os
import sys
import re

def parse_input_file(fname):
    out = {'directories':[],'extensions':[],'files':[],'exclude folder names':[]}
    current_section = ''
    with open(fname,'r') as infile:
        for line in infile:
            line = line.strip()
            m = re.search(r'\s*(.*):',line)
            line_is_section = False
            if m:
                section_name = m.group(1).lower()
                if section_name in out:
                    current_section = section_name
                    line_is_section = True
            if current_section and not line_is_section and line:
                out[current_section].append(line)
    return out
    
def words_not_in_str(wdlist,s):
    out = True
    for wd in wdlist:
        if '/'+wd in s:
            return False
        if '\\'+wd in s:
            return False
    return out
    
def relative_path(proj_dir,path):
    out = path.replace(proj_dir,'')
    if out:
        if out[0] == '\\' or out[0] == '/':
            out = out[1:]
    return out

def main(fname,output_directory,output_fname):
    output_data = []
    input_data = parse_input_file(fname)
    for proj_dir in input_data['directories']:
        main_dir_name = os.path.split(proj_dir)[1]
        for path, dirs, files in os.walk(proj_dir):
             for file in files:
                extension = pathlib.Path(file).suffix[1:]
                if file in input_data['files'] or\
                    (words_not_in_str(input_data['exclude folder names'],path) and\
                    extension in input_data['extensions']):
                    output_data.append({
                        'input':os.path.join(path, file).replace('/','\\'),
                        'output':os.path.join(output_directory, main_dir_name, relative_path(proj_dir,path), file).replace('/','\\')
                    })
    with open(output_fname,'w') as outfile:
        for od in output_data:
            outfile.write('echo f | xcopy \"'+od['input']+'\" \"'+od['output']+'\" /y\n')

if __name__ == "__main__":
    if os.name == 'nt':
        if len(sys.argv) > 2:
            main(sys.argv[1],sys.argv[2],sys.argv[3] if len(sys.argv) > 3 else 'copy_project_files.bat')
        else:
            print('format: python generate_file_copy_bat.py <file with list of project paths> <destination directory> <output file path>')
    else:
        print('This script is presently for Windows systems only')