/*
WolfBox Fantasy Workstation
Copyright 2022-2022 Talon396

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

extern crate pest;
#[macro_use]
extern crate pest_derive;

use std::env;
use std::process::exit;
use lazy_static::lazy_static;
use std::sync::Mutex;

lazy_static! {
    static ref SOURCE_PATH: Mutex<std::path::PathBuf> = Mutex::new(std::path::PathBuf::new());
}

#[derive(Parser)]
#[grammar = "icewolf.pest"]
struct IceWolfParser;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        println!("Usage: icewolf-as <infile> <outfile>");
        exit(1);
    }
    let mut infile = std::fs::read_to_string(&args[1]).expect("Unable to read file!");
    println!("Running preprocessor...");
    let mut source_path = std::fs::canonicalize(&args[1]).unwrap();
    source_path.pop();
    *SOURCE_PATH.lock().unwrap() = source_path;
    for _ in 0..128 {
        let loop_file = infile.clone(); // this is a hack to allow modifying input_file from inside the for loop
        for (line_number, text) in loop_file.lines().enumerate() {
            match text.trim() {
                s if s.starts_with(".include \"") => {
                    infile = include(line_number, text.trim(), infile);
                    break;
                },
                _ => {}
            };
        }
    }
}

fn include(line_number: usize, text: &str, data: String) -> String {
    let path_start_index = text.find("\"").unwrap() + 1;
    let path_end_index = text.len() - 1;
    let path_string = &text[path_start_index..path_end_index];
    let mut source_path = SOURCE_PATH.lock().unwrap().clone();
    source_path.push(path_string);
    let mut start_of_original_file = String::new();
    for (i, text) in data.lines().enumerate() {
        if i < line_number {
            start_of_original_file.push_str(text);
            start_of_original_file.push('\n');
        }
    }
    let mut included_file = std::fs::read_to_string(source_path).expect("failed to include file");
    included_file.push('\n');
    let mut end_of_original_file = String::new();
    for (i, text) in data.lines().enumerate() {
        if i > line_number {
            end_of_original_file.push_str(text);
            end_of_original_file.push('\n');
        }
    }
    let mut final_file = String::new();
    final_file.push_str(&start_of_original_file);
    final_file.push_str(&included_file);
    final_file.push_str(&end_of_original_file);
    final_file
}