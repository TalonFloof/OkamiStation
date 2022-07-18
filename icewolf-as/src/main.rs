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
use pest::Parser;

lazy_static! {
    static ref SOURCE_PATH: Mutex<std::path::PathBuf> = Mutex::new(std::path::PathBuf::new());
}

#[derive(Parser)]
#[grammar = "icewolf.pest"]
struct IceWolfParser;


#[derive(PartialEq, Debug, Clone)]
#[allow(dead_code)]
enum ASTEntry {
    Instruction0Arg {
        instruction: Instructions0Arg,
    },
    Instruction1Arg {
        instruction: Instructions1Arg,
        arg1: Box<ASTEntry>,
    },
    Instruction2Arg {
        instruction: Instructions2Arg,
        arg1: Box<ASTEntry>,
        arg2: Box<ASTEntry>,
    },
    Instruction3Arg {
        instruction: Instructions3Arg,
        arg1: Box<ASTEntry>,
        arg2: Box<ASTEntry>,
        arg3: Box<ASTEntry>,
    },
    ImmediateByte(u8),
    ImmediateHalf(u16),
    ImmediateWord(u32),
    ImmediateLong(u64),
    Register(u8),
    LabelRef {
        name: String,
        is_absolute: bool,
    },
    LabelDefine {
        name: String,
        is_extern: bool,
        is_global: bool,
    },
    Origin {
        has_padding: bool,
        base: u64,
    },
    DataNum(Box<ASTEntry>),
    DataString(String),
    DataFilling {
        value: u8,
        size: u64,
    },
}


#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum Instructions0Arg {
    Tret,
    Nop
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum Instructions1Arg {
    Fence,
    Lui,
    ECall,
    Ba,
    B,
    As,
    Add,
    Sub,
    Xor,
    Or,
    And,
    Sll,
    Srl,
    Sra,
    Slt,
    Sltu,
    Mul,
    Mulh,
    Mulsu,
    Mulu,
    Div,
    Divu,
    Rem,
    Remu,
    Lb,
    Lh,
    Lw,
    Ld,
    Sb,
    Sh,
    Sw,
    Sd
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum Instructions2Arg {
    Mov,
    Wrxreg,
    Rdxreg,
    Swpxreg,
    Bal,
    Bala
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum Instructions3Arg {
    Beq,
    Bne,
    Blt,
    Bge,
    Bltu,
    Bgeu
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        println!("Usage: icewolf-as <infile> <outfile>");
        exit(1);
    }
    let mut infile = std::fs::read_to_string(&args[1]).expect("Unable to read file!");
    println!("Running preprocessor");
    let mut source_path = std::fs::canonicalize(&args[1]).unwrap();
    source_path.pop();
    *SOURCE_PATH.lock().unwrap() = source_path;
    for _ in 0..128 {
        let loop_file = infile.clone();
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
    println!("Running AST Parsing");
    let ast = parse(&infile);
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
    let mut included_file = std::fs::read_to_string(source_path).expect("File Include Failed!");
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

fn parse(source: &str) -> Result<Vec<ASTEntry>, ()> {
    let mut ast: Vec<ASTEntry> = vec![];
    let pairs = IceWolfParser::parse(Rule::program, source).expect("\"pest\" Panicked during parsing");
    for pair in pairs.peek().unwrap().into_inner() {
        match pair.as_rule() {
            Rule::EOI => break,
            _ => {
                let ent = pair_to_ast(pair);
                println!("Parsed: {:?}", ent);
                ast.push(ent);
            }
        }
    }
    Ok(ast)
}

fn splice_underscores(s: &str) -> String {
    String::from_iter(s.chars().filter(|c| *c != '_'))
}

fn pair_to_ast(pair: pest::iterators::Pair<Rule>) -> ASTEntry {
    match pair.as_rule() {
        Rule::program => pair_to_ast(pair.into_inner().next().unwrap()),
        Rule::label => ASTEntry::LabelDefine { name:pair.into_inner().next().unwrap().as_str().to_string(), is_extern: false, is_global: false },
        Rule::instruction => {
            let mut inner_pair = pair.into_inner();
            match inner_pair.peek().unwrap().as_rule() {
                Rule::instruction_0arg => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    return ASTEntry::Instruction0Arg {
                        instruction: match val1 {
                            "tret" => Instructions0Arg::Tret,
                            "nop" => Instructions0Arg::Nop,
                            _ => todo!(),
                        }
                    }
                }
                Rule::instruction_1arg => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    return ASTEntry::Instruction1Arg {
                        instruction: match val1 {
                            "fence" => Instructions1Arg::Fence,
                            "lui" => Instructions1Arg::Lui,
                            "ecall" => Instructions1Arg::ECall,
                            "ba" => Instructions1Arg::Ba,
                            "b" => Instructions1Arg::B,
                            "as" => Instructions1Arg::As,
                            "add" => Instructions1Arg::Add,
                            "sub" => Instructions1Arg::Sub,
                            "xor" => Instructions1Arg::Xor,
                            "or" => Instructions1Arg::Or,
                            "and" => Instructions1Arg::And,
                            "sll" => Instructions1Arg::Sll,
                            "srl" => Instructions1Arg::Srl,
                            "sra" => Instructions1Arg::Sra,
                            "slt" => Instructions1Arg::Slt,
                            "sltu" => Instructions1Arg::Sltu,
                            "mul" => Instructions1Arg::Mul,
                            "mulh" => Instructions1Arg::Mulh,
                            "mulsu" => Instructions1Arg::Mulsu,
                            "mulu" => Instructions1Arg::Mulu,
                            "div" => Instructions1Arg::Div,
                            "divu" => Instructions1Arg::Divu,
                            "rem" => Instructions1Arg::Rem,
                            "remu" => Instructions1Arg::Remu,
                            "lb" => Instructions1Arg::Lb,
                            "lh" => Instructions1Arg::Lh,
                            "lw" => Instructions1Arg::Lw,
                            "ld" => Instructions1Arg::Ld,
                            "sb" => Instructions1Arg::Sb,
                            "sh" => Instructions1Arg::Sh,
                            "sw" => Instructions1Arg::Sw,
                            "sd" => Instructions1Arg::Sd,
                            _ => todo!(),
                        },
                        arg1: Box::new(pair_to_ast(val2)),
                    }
                }
                Rule::instruction_2arg => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    let val3 = inner_pair.next().unwrap();
                    return ASTEntry::Instruction2Arg {
                        instruction: match val1 {
                            "mov" => Instructions2Arg::Mov,
                            "wrxreg" => Instructions2Arg::Wrxreg,
                            "rdxreg" => Instructions2Arg::Rdxreg,
                            "swpxreg" => Instructions2Arg::Swpxreg,
                            "bal" => Instructions2Arg::Bal,
                            "bala" => Instructions2Arg::Bala,
                            _ => todo!(),
                        },
                        arg1: Box::new(pair_to_ast(val2)),
                        arg2: Box::new(pair_to_ast(val3)),
                    }
                }
                Rule::instruction_3arg => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    let val3 = inner_pair.next().unwrap();
                    let val4 = inner_pair.next().unwrap();
                    return ASTEntry::Instruction3Arg {
                        instruction: match val1 {
                            "beq" => Instructions3Arg::Beq,
                            "bne" => Instructions3Arg::Bne,
                            "blt" => Instructions3Arg::Blt,
                            "bge" => Instructions3Arg::Bge,
                            "bltu" => Instructions3Arg::Bltu,
                            "bgeu" => Instructions3Arg::Bgeu,
                            _ => todo!(),
                        },
                        arg1: Box::new(pair_to_ast(val2)),
                        arg2: Box::new(pair_to_ast(val3)),
                        arg3: Box::new(pair_to_ast(val4)),
                    }
                }
                _ => panic!("Invalid instruction type"),
            }
        }
        Rule::operand => {
            let val = pair.into_inner().next().unwrap();
            match val.as_rule() {
                Rule::immediate_bin => {
                    let num = u64::from_str_radix(&splice_underscores(val.into_inner().next().unwrap().as_str()), 2).unwrap();
                    let len = 64-num.leading_zeros();
                    if (0..=8).contains(&len) {
                        return ASTEntry::ImmediateByte(num as u8);
                    } else if (9..=16).contains(&len) {
                        return ASTEntry::ImmediateHalf(num as u16);
                    } else if (17..=32).contains(&len) {
                        return ASTEntry::ImmediateWord(num as u32);
                    } else {
                        return ASTEntry::ImmediateLong(num);
                    }
                }
                Rule::immediate_hex => {
                    let num = u64::from_str_radix(&splice_underscores(val.into_inner().next().unwrap().as_str()), 16).unwrap();
                    let len = 64-num.leading_zeros();
                    if (0..=8).contains(&len) {
                        return ASTEntry::ImmediateByte(num as u8);
                    } else if (9..=16).contains(&len) {
                        return ASTEntry::ImmediateHalf(num as u16);
                    } else if (17..=32).contains(&len) {
                        return ASTEntry::ImmediateWord(num as u32);
                    } else {
                        return ASTEntry::ImmediateLong(num);
                    }
                }
                Rule::immediate_dec => {
                    let num = u64::from_str_radix(&splice_underscores(val.into_inner().next().unwrap().as_str()), 10).unwrap();
                    let len = 64-num.leading_zeros();
                    if (0..=8).contains(&len) {
                        return ASTEntry::ImmediateByte(num as u8);
                    } else if (9..=16).contains(&len) {
                        return ASTEntry::ImmediateHalf(num as u16);
                    } else if (17..=32).contains(&len) {
                        return ASTEntry::ImmediateWord(num as u32);
                    } else {
                        return ASTEntry::ImmediateLong(num);
                    }
                }
                Rule::label_ref => {
                    return ASTEntry::LabelRef {
                        name: val.into_inner().next().unwrap().as_str().to_string(),
                        is_absolute: false,
                    }
                }
                Rule::label_abs_ref => {
                    return ASTEntry::LabelRef {
                        name: val.into_inner().next().unwrap().as_str().to_string(),
                        is_absolute: true,
                    }
                }
                Rule::register => {
                    let val = val.into_inner().next().unwrap().as_str();
                    return ASTEntry::Register(match val {
                        "x0"|"pc" => 0x00,
                        "x1"|"ra" => 0x01,
                        "x2"|"sp" => 0x02,
                        "x3"|"gp" => 0x03,
                        "x4"|"tp" => 0x04,
                        "x5"|"t0" => 0x05,
                        "x6"|"t1" => 0x06,
                        "x7"|"t2" => 0x07,
                        "x8"|"s0"|"fp" => 0x08,
                        "x9"|"s1" => 0x09,
                        "x10"|"a0" => 0x0a,
                        "x11"|"a1" => 0x0b,
                        "x12"|"a2" => 0x0c,
                        "x13"|"a3" => 0x0d,
                        "x14"|"a4" => 0x0e,
                        "x15"|"a5" => 0x0f,
                        "x16"|"a6" => 0x10,
                        "x17"|"a7" => 0x11,
                        "x18"|"s2" => 0x12,
                        "x19"|"s3" => 0x13,
                        "x20"|"s4" => 0x14,
                        "x21"|"s5" => 0x15,
                        "x22"|"s6" => 0x16,
                        "x23"|"s7" => 0x17,
                        "x24"|"s8" => 0x18,
                        "x25"|"s9" => 0x19,
                        "x26"|"s10" => 0x1a,
                        "x27"|"s11" => 0x1b,
                        "x28"|"t3" => 0x1c,
                        "x29"|"t4" => 0x1d,
                        "x30"|"t5" => 0x1e,
                        "x31"|"t6" => 0x1f,
                        "alu.acc" => 0x20,
                        "alu.add" => 0x21,
                        "alu.sub" => 0x22,
                        "alu.xor" => 0x23,
                        "alu.or" => 0x24,
                        "alu.and" => 0x25,
                        "alu.sll" => 0x26,
                        "alu.srl" => 0x27,
                        "alu.sra" => 0x28,
                        "alu.slt" => 0x29,
                        "alu.sltu" => 0x2a,
                        "alu.mul" => 0x2b,
                        "alu.mulh" => 0x2c,
                        "alu.mulsu" => 0x2d,
                        "alu.mulu" => 0x2e,
                        "alu.div" => 0x2f,
                        "alu.divu" => 0x30,
                        "alu.rem" => 0x31,
                        "alu.remu" => 0x32,
                        "zero" => 0x3f,
                        "mem.lb" => 0x40,
                        "mem.lh" => 0x41,
                        "mem.lw" => 0x42,
                        "mem.ld" => 0x43,
                        "mem.sb" => 0x44,
                        "mem.sh" => 0x45,
                        "mem.sw" => 0x46,
                        "mem.sd" => 0x47,
                        "mem.fence" => 0x4f,
                        "bran.eq" => 0x50,
                        "bran.ne" => 0x51,
                        "bran.lt" => 0x52,
                        "bran.ge" => 0x53,
                        "bran.ltu" => 0x54,
                        "bran.geu" => 0x55,
                        "bran.lnk" => 0x56,
                        "bran.rel" => 0x57,
                        "bran.rln" => 0x58,
                        "trapret" => 0x5a,
                        "ecall" => 0x5b,
                        "xreg.ind" => 0x5c,
                        "xreg.val" => 0x5d,
                        "xreg.swp" => 0x5e,
                        "lui" => 0x5f,
                        _ => panic!("Invalid register")
                    })
                }
                _ => panic!("Invalid operand type"),
            }
        }
        Rule::origin => {
            let val = pair.into_inner().next().unwrap();
            match val.as_rule() {
                Rule::origin_without_padding => {
                    let ast = pair_to_ast(val.into_inner().next().unwrap());
                    match ast {
                        ASTEntry::ImmediateByte(num) => {
                            return ASTEntry::Origin { has_padding: false, base: num as u64 }
                        }
                        ASTEntry::ImmediateHalf(num) => {
                            return ASTEntry::Origin { has_padding: false, base: num as u64 }
                        }
                        ASTEntry::ImmediateWord(num) => {
                            return ASTEntry::Origin { has_padding: false, base: num as u64 }
                        }
                        ASTEntry::ImmediateLong(num) => {
                            return ASTEntry::Origin { has_padding: false, base: num }
                        }
                        _ => panic!("Invalid operand type for origin"),
                    }
                }
                Rule::origin_with_padding => {
                    let ast = pair_to_ast(val.into_inner().next().unwrap());
                    match ast {
                        ASTEntry::ImmediateByte(num) => {
                            return ASTEntry::Origin { has_padding: true, base: num as u64 }
                        }
                        ASTEntry::ImmediateHalf(num) => {
                            return ASTEntry::Origin { has_padding: true, base: num as u64 }
                        }
                        ASTEntry::ImmediateWord(num) => {
                            return ASTEntry::Origin { has_padding: true, base: num as u64 }
                        }
                        ASTEntry::ImmediateLong(num) => {
                            return ASTEntry::Origin { has_padding: true, base: num }
                        }
                        _ => panic!("Invalid operand type for origin w/ padding"),
                    }
                }
                _ => panic!("Invalid origin type"),
            }
        }
        Rule::data => {
            let val = pair.into_inner().next().unwrap();
            match val.as_rule() {
                Rule::data_byte|Rule::data_half|Rule::data_word|Rule::data_long => {
                    return ASTEntry::DataNum(Box::new(pair_to_ast(val.into_inner().next().unwrap())))
                }
                Rule::data_str => {
                    let s = val.into_inner().next().unwrap().into_inner().next().unwrap().as_str();
                    return ASTEntry::DataString(String::from(s))
                }
                Rule::data_fill => {
                    let mut inner = val.into_inner();
                    let arg1 = pair_to_ast(inner.next().unwrap());
                    let arg2 = pair_to_ast(inner.next().unwrap());
                    if let ASTEntry::ImmediateByte(num) = arg1 {
                        match arg2 {
                            ASTEntry::ImmediateByte(size) => {
                                return ASTEntry::DataFilling { value: num, size: size as u64 }
                            }
                            ASTEntry::ImmediateHalf(size) => {
                                return ASTEntry::DataFilling { value: num, size: size as u64 }
                            }
                            ASTEntry::ImmediateWord(size) => {
                                return ASTEntry::DataFilling { value: num, size: size as u64 }
                            }
                            ASTEntry::ImmediateLong(size) => {
                                return ASTEntry::DataFilling { value: num, size: size }
                            }
                            _ => panic!("Invalid operand type for filling")
                        }
                    } else {
                        panic!("Value for filling exceeds 8-bit boundary");
                    }
                }
                _ => panic!("Invalid data type"),
            }
        }
        _ => panic!("Unsupported type!")
    }
}