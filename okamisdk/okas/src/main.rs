// This uses some source code from my old icewolf-as assembler, so it's not the cleanest...
// This also uses some code from ry's fox32asm, which can be found here: https://github.com/fox32-arch/fox32asm/blob/main/src/main.rs

extern crate pest;
#[macro_use]
extern crate pest_derive;
extern crate lazy_static;

use lazy_static::lazy_static;
use pest::Parser;
use std::env;
use std::process::exit;
use std::sync::Mutex;

lazy_static! {
    static ref SOURCE_PATH: Mutex<std::path::PathBuf> = Mutex::new(std::path::PathBuf::new());
}

#[derive(Parser)]
#[grammar = "okami.pest"]
struct OkamiParser;

#[derive(PartialEq, Debug, Clone)]
#[allow(dead_code)]
enum LabelRefType {
    Branch28,
    Ptr16,
    Ptr32,
    La32,
}

#[derive(PartialEq, Debug, Clone)]
#[allow(dead_code)]
enum ASTNode {
    InstructionZero {
        op: InstructionZero,
    },
    InstructionOne {
        op: InstructionOne,
        operand: Box<ASTNode>,
    },
    InstructionTwo {
        op: InstructionTwo,
        operand1: Box<ASTNode>,
        operand2: Box<ASTNode>,
    },
    InstructionTwoMemory {
        op: InstructionTwoMemory,
        operand1: Box<ASTNode>,
        offset: Box<ASTNode>,
        operand2: Box<ASTNode>,
    },
    InstructionThree {
        op: InstructionThree,
        operand1: Box<ASTNode>,
        operand2: Box<ASTNode>,
        operand3: Box<ASTNode>,
    },
    InstructionFour {
        op: InstructionFour,
        operand1: Box<ASTNode>,
        operand2: Box<ASTNode>,
        operand3: Box<ASTNode>,
        operand4: Box<ASTNode>,
    },
    ImmediateByte(u8),
    ImmediateHalf(u16),
    ImmediateWord(u32),
    Register(u8),
    LabelRef {
        name: String,
        reftype: LabelRefType,
    },
    LabelDefine {
        name: String,
        is_local: bool,
        is_extern: bool,
        is_global: bool,
    },
    BinaryInclude(String),
    DataNum(Box<ASTNode>),
    DataString(String),
    Section(SectionType),
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum SectionType {
    Text,
    RoData,
    Data,
    Bss,
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum InstructionZero {
    Nop, /* PSEUDO-MNEMONIC */
    Rft,
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum InstructionOne {
    KCall,
    B,
    Bl,
    Br, /* PSEUDO-MNEMONIC */
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum InstructionTwo {
    Lui,
    Mfex,
    Mtex,
    Aupc,
    Blr,
    La,
    Li,
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum InstructionTwoMemory {
    Lb,
    Lh,
    Lw,
    Lbu,
    Lhu,
    Sb,
    Sh,
    Sw,
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum InstructionThree {
    Add,
    Addi,
    Sub,
    And,
    Andi,
    Or,
    Ori,
    Xor,
    Xori,
    Sll,
    Slli,
    Srl,
    Srli,
    Sra,
    Srai,
    Slt,
    Slti,
    Sltu,
    Sltiu,
    Beq,
    Bne,
    Bge,
    Blt,
    Bgeu,
    Bltu,
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum InstructionFour {
    Mul,
    Mulu,
    Div,
    Divu,
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        println!("Usage: okas [infile] [outfile]");
        exit(1);
    }
    let start_time = std::time::Instant::now();
    let mut infile = std::fs::read_to_string(&args[1]).expect("Unable to read file!");
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
                }
                _ => {}
            };
        }
    }
    let ast_nodes = parse(&infile);
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

fn parse(source: &str) -> Result<Vec<ASTNode>, ()> {
    let mut ast: Vec<ASTNode> = vec![];
    let pairs = OkamiParser::parse(Rule::assembly, source).expect("");
    for pair in pairs.peek().unwrap().into_inner() {
        match pair.as_rule() {
            Rule::EOI => break,
            _ => {
                ast.push(to_ast_symbol(pair));
            }
        }
    }
    Ok(ast)
}

fn to_ast_symbol(pair: pest::iterators::Pair<Rule>) -> ASTNode {
    match pair.as_rule() {
        Rule::assembly => to_ast_symbol(pair.into_inner().next().unwrap()),
        Rule::include_bin => {
            let inner = pair.into_inner().next().unwrap();
            match inner.as_rule() {
                Rule::imm_str => {
                    let s = inner.into_inner().next().unwrap().as_str();
                    return ASTNode::BinaryInclude(String::from(s));
                }
                _ => todo!(),
            }
        }
        Rule::instruction => {
            let mut inner_pair = pair.into_inner();
            match inner_pair.peek().unwrap().as_rule() {
                Rule::opcode_0 => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    return ASTNode::InstructionZero {
                        op: match val1 {
                            "nop" => InstructionZero::Nop,
                            "rft" => InstructionZero::Rft,
                            _ => todo!(),
                        },
                    };
                }
                Rule::opcode_1 => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    return ASTNode::InstructionOne {
                        op: match val1 {
                            "kcall" => InstructionOne::KCall,
                            "b" => InstructionOne::B,
                            "bl" => InstructionOne::Bl,
                            "br" => InstructionOne::Br,
                            _ => todo!(),
                        },
                        operand: Box::new(to_ast_symbol(val2)),
                    };
                }
                Rule::opcode_2 => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    let val3 = inner_pair.next().unwrap();
                    return ASTNode::InstructionTwo {
                        op: match val1 {
                            "lui" => InstructionTwo::Lui,
                            "mfex" => InstructionTwo::Mfex,
                            "mtex" => InstructionTwo::Mtex,
                            "aupc" => InstructionTwo::Aupc,
                            "blr" => InstructionTwo::Blr,
                            "la" => InstructionTwo::La,
                            "li" => InstructionTwo::Li,
                            _ => todo!(),
                        },
                        operand1: Box::new(to_ast_symbol(val2)),
                        operand2: Box::new(to_ast_symbol(val3)),
                    };
                }
                Rule::opcode_2_mem => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    let val3 = inner_pair.next().unwrap();
                    let val4 = inner_pair.next().unwrap();
                    return ASTNode::InstructionTwoMemory {
                        op: match val1 {
                            "lbu" => InstructionTwoMemory::Lbu,
                            "lhu" => InstructionTwoMemory::Lhu,
                            "lb" => InstructionTwoMemory::Lb,
                            "lh" => InstructionTwoMemory::Lh,
                            "lw" => InstructionTwoMemory::Lw,
                            "sb" => InstructionTwoMemory::Sb,
                            "sh" => InstructionTwoMemory::Sh,
                            "sw" => InstructionTwoMemory::Sw,
                            _ => todo!(),
                        },
                        operand1: Box::new(to_ast_symbol(val2)),
                        offset: Box::new(to_ast_symbol(val4)),
                        operand2: Box::new(to_ast_symbol(val3)),
                    };
                }
                Rule::opcode_3 => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    let val3 = inner_pair.next().unwrap();
                    let val4 = inner_pair.next().unwrap();
                    return ASTNode::InstructionThree {
                        op: match val1 {
                            "addi" => InstructionThree::Addi,
                            "add" => InstructionThree::Add,
                            "sub" => InstructionThree::Sub,
                            "andi" => InstructionThree::Andi,
                            "and" => InstructionThree::And,
                            "ori" => InstructionThree::Ori,
                            "or" => InstructionThree::Or,
                            "xori" => InstructionThree::Xori,
                            "xor" => InstructionThree::Xor,
                            "slli" => InstructionThree::Slli,
                            "sll" => InstructionThree::Sll,
                            "srli" => InstructionThree::Srli,
                            "srl" => InstructionThree::Srl,
                            "srai" => InstructionThree::Srai,
                            "sra" => InstructionThree::Sra,
                            "sltu" => InstructionThree::Sltu,
                            "sltiu" => InstructionThree::Sltiu,
                            "slti" => InstructionThree::Slti,
                            "slt" => InstructionThree::Slt,
                            "beq" => InstructionThree::Beq,
                            "bne" => InstructionThree::Bne,
                            "bgeu" => InstructionThree::Bgeu,
                            "bltu" => InstructionThree::Bltu,
                            "bge" => InstructionThree::Bge,
                            "blt" => InstructionThree::Blt,
                            _ => todo!(),
                        },
                        operand1: Box::new(to_ast_symbol(val2)),
                        operand2: Box::new(to_ast_symbol(val3)),
                        operand3: Box::new(to_ast_symbol(val4)),
                    };
                }
                Rule::opcode_4 => {
                    let val1 = inner_pair.next().unwrap().as_str();
                    let val2 = inner_pair.next().unwrap();
                    let val3 = inner_pair.next().unwrap();
                    let val4 = inner_pair.next().unwrap();
                    let val5 = inner_pair.next().unwrap();
                    return ASTNode::InstructionFour {
                        op: match val1 {
                            "mulu" => InstructionFour::Mulu,
                            "mul" => InstructionFour::Mul,
                            "divu" => InstructionFour::Divu,
                            "div" => InstructionFour::Div,
                            _ => todo!(),
                        },
                        operand1: Box::new(to_ast_symbol(val2)),
                        operand2: Box::new(to_ast_symbol(val3)),
                        operand3: Box::new(to_ast_symbol(val4)),
                        operand4: Box::new(to_ast_symbol(val5)),
                    };
                }
                _ => todo!(),
            }
        }
        Rule::section => {
            return match pair.as_str() {
                ".text" => ASTNode::Section(SectionType::Text),
                ".rodata" => ASTNode::Section(SectionType::RoData),
                ".data" => ASTNode::Section(SectionType::Data),
                ".bss" => ASTNode::Section(SectionType::Bss),
                _ => todo!(),
            }
        }
        Rule::label => {
            println!("{}", pair.into_inner().next().unwrap());
            todo!();
        }
        _ => todo!(),
    }
}
