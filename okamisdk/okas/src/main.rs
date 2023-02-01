// This uses some source code from my old icewolf-as assembler, so it's not the cleanest...
// This also uses some code from ry's fox32asm, which can be found here: https://github.com/fox32-arch/fox32asm/blob/main/src/main.rs

extern crate pest;
#[macro_use]
extern crate pest_derive;
extern crate lazy_static;

use lazy_static::lazy_static;
use pest::Parser;
use std::collections::BTreeMap;
use std::env;
use std::process::exit;
use std::sync::Mutex;

lazy_static! {
    static ref SOURCE_PATH: Mutex<std::path::PathBuf> = Mutex::new(std::path::PathBuf::new());
    static ref LAST_GLOBAL_LABEL: Mutex<String> = Mutex::new(String::new());
}

#[derive(Parser)]
#[grammar = "okami.pest"]
struct OkamiParser;

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
    Immediate(u32),
    Register(u8),
    LabelRef {
        name: String,
        is_local: bool,
    },
    LabelDefine {
        name: String,
        is_local: bool,
        is_extern: bool,
        is_global: bool,
    },
    BinaryInclude(Vec<u8>),
    DataNum(u8, Box<ASTNode>),
    DataString(String),
    ExtendBSS(u32),
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

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
enum RelocationType {
    Branch28 = 0,
    Ptr32 = 1,
    La32 = 2,
    Rel16 = 3,
}

#[derive(PartialEq, Debug, Clone, Copy)]
#[allow(dead_code)]
struct RelocationEntry {
    section: SectionType,
    offset: u32,
    reloc_type: RelocationType,
}

#[derive(PartialEq, Debug, Clone)]
#[allow(dead_code)]
struct Segments {
    text: Vec<u8>,
    rodata: Vec<u8>,
    data: Vec<u8>,
    bss: u32,
    labels: BTreeMap<String, (SectionType, u32, bool)>,
    reloc: BTreeMap<String, Vec<RelocationEntry>>,
}

impl Segments {
    pub fn new() -> Self {
        return Self {
            text: Vec::new(),
            rodata: Vec::new(),
            data: Vec::new(),
            bss: 0,
            labels: BTreeMap::new(),
            reloc: BTreeMap::new(),
        };
    }
    pub fn push(&mut self, segment: SectionType, data: &[u8]) {
        match segment {
            SectionType::Text => {
                self.text.extend_from_slice(data);
            }
            SectionType::RoData => {
                self.rodata.extend_from_slice(data);
            }
            SectionType::Data => {
                self.data.extend_from_slice(data);
            }
            _ => todo!(),
        }
    }
    pub fn push32(&mut self, segment: SectionType, data: u32) {
        self.push(segment, data.to_le_bytes().as_slice());
    }
    pub fn push_vec(&mut self, segment: SectionType, data: &mut Vec<u8>) {
        match segment {
            SectionType::Text => {
                self.text.append(data);
            }
            SectionType::RoData => {
                self.rodata.append(data);
            }
            SectionType::Data => {
                self.data.append(data);
            }
            _ => todo!(),
        }
    }
    pub fn add_reloc_entry(
        &mut self,
        segment: SectionType,
        name: String,
        reloc_type: RelocationType,
    ) {
        let seg_size = self.get_size(segment);
        if let Some(val) = self.reloc.get_mut(&name) {
            val.push(RelocationEntry {
                section: segment,
                offset: seg_size,
                reloc_type,
            });
        } else {
            let mut entries = Vec::new();
            entries.push(RelocationEntry {
                section: segment,
                offset: seg_size,
                reloc_type,
            });
            self.reloc.insert(name, entries);
        }
    }
    pub fn extend_bss(&mut self, size: u32) {
        self.bss += size;
    }
    pub fn get_size(&self, segment: SectionType) -> u32 {
        return match segment {
            SectionType::Text => self.text.len() as u32,
            SectionType::RoData => self.rodata.len() as u32,
            SectionType::Data => self.data.len() as u32,
            SectionType::Bss => self.bss,
        };
    }
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
    let ast_nodes = parse(&infile).unwrap();
    drop(infile);
    // Time to start assembly
    let mut current_section = SectionType::Text;
    let mut segments = Segments::new();
    for node in ast_nodes {
        match node {
            ASTNode::Section(section) => {
                current_section = section;
            }
            ASTNode::LabelDefine {
                name,
                is_local: _,
                is_extern: _,
                is_global: glob,
            } => {
                segments.labels.insert(
                    name.clone(),
                    (current_section, segments.get_size(current_section), glob),
                );
            }
            ASTNode::BinaryInclude(mut val) => {
                segments.push_vec(current_section, &mut val);
            }
            ASTNode::ExtendBSS(size) => {
                segments.extend_bss(size);
            }
            ASTNode::DataNum(size, num) => {
                if let ASTNode::Immediate(val) = *num {
                    if size == 1 {
                        segments.push(current_section, [val.to_le_bytes()[0]].as_slice());
                    } else if size == 2 {
                        segments.push(current_section, &val.to_le_bytes().as_slice()[0..=1]);
                    } else if size == 4 {
                        segments.push(current_section, val.to_le_bytes().as_slice());
                    }
                }
            }
            ASTNode::DataString(str) => {
                segments.push(current_section, str.as_bytes());
            }
            ASTNode::InstructionZero { op } => match op {
                InstructionZero::Nop => segments.push32(current_section, 0), // add zero, zero, zero
                InstructionZero::Rft => segments.push32(current_section, 0xFC000000),
            },
            ASTNode::InstructionOne { op, operand: arg1 } => match op {
                InstructionOne::KCall => {
                    if let ASTNode::Immediate(num) = *arg1 {
                        segments.push32(current_section, 0xF8000000 | (num & 0x3FFFFFF));
                    } else {
                        panic!("KCall with non integer operand.");
                    }
                }
                InstructionOne::B => {
                    if let ASTNode::LabelRef {
                        name: label,
                        is_local: _,
                    } = *arg1
                    {
                        segments.add_reloc_entry(current_section, label, RelocationType::Branch28);
                        segments.push32(current_section, 0x80000000);
                    } else {
                        panic!("Branch with non label operand.");
                    }
                }
                InstructionOne::Bl => {
                    if let ASTNode::LabelRef {
                        name: label,
                        is_local: _,
                    } = *arg1
                    {
                        segments.add_reloc_entry(current_section, label, RelocationType::Branch28);
                        segments.push32(current_section, 0x84000000);
                    } else {
                        panic!("Linked Branch with non label operand.");
                    }
                }
                InstructionOne::Br => {
                    if let ASTNode::Register(reg) = *arg1 {
                        segments.push32(current_section, 0x88000000 | ((reg as u32) << 21));
                    } else {
                        panic!("Register Branch with non register operand.");
                    }
                }
            },
            ASTNode::InstructionTwo {
                op,
                operand1: arg1,
                operand2: arg2,
            } => match op {
                InstructionTwo::Lui => {
                    if let ASTNode::Register(reg) = *arg1 {
                        if let ASTNode::Immediate(val) = *arg2 {
                            segments.push32(
                                current_section,
                                0x60000000 | ((reg as u32) << 16) | (val & 0xFFFF),
                            );
                        }
                    }
                }
                InstructionTwo::Mfex => {
                    if let ASTNode::Register(reg) = *arg1 {
                        if let ASTNode::Immediate(val) = *arg2 {
                            segments.push32(
                                current_section,
                                0xF0000000 | ((reg as u32) << 21) | (val & 0xFFFF),
                            );
                        }
                    }
                }
                InstructionTwo::Mtex => {
                    if let ASTNode::Register(reg) = *arg1 {
                        if let ASTNode::Immediate(val) = *arg2 {
                            segments.push32(
                                current_section,
                                0xF4000000 | ((reg as u32) << 16) | (val & 0xFFFF),
                            );
                        }
                    }
                }
                InstructionTwo::Blr => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            segments.push32(
                                current_section,
                                0x88000000 | ((reg1 as u32) << 16) | ((reg2 as u32) << 21),
                            );
                        }
                    }
                }
                InstructionTwo::La => {
                    if let ASTNode::Register(reg) = *arg1 {
                        if let ASTNode::Immediate(val) = *arg2 {
                            segments.push32(
                                current_section,
                                0x60000000 | ((reg as u32) << 16) | (val & 0xFFFF),
                            );
                            segments.push32(
                                current_section,
                                0x48000000 | ((reg as u32) << 16) | ((val & 0xFFFF0000) >> 16),
                            );
                        } else if let ASTNode::LabelRef {
                            name: label,
                            is_local: _,
                        } = *arg2
                        {
                            segments.add_reloc_entry(current_section, label, RelocationType::La32);
                            segments.push32(current_section, 0x60000000 | ((reg as u32) << 16));
                            segments.push32(current_section, 0x48000000 | ((reg as u32) << 16));
                        }
                    }
                }
                InstructionTwo::Li => {
                    if let ASTNode::Register(reg) = *arg1 {
                        if let ASTNode::Immediate(val) = *arg2 {
                            segments.push32(
                                current_section,
                                0x48000000 | ((reg as u32) << 16) | (val & 0xFFFF),
                            );
                        }
                    }
                }
            },
            ASTNode::InstructionTwoMemory {
                op,
                operand1: arg1,
                offset: off,
                operand2: arg2,
            } => match op {
                InstructionTwoMemory::Lbu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xC4000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionTwoMemory::Lhu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xCC000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionTwoMemory::Lb => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xC0000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionTwoMemory::Lh => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xC8000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionTwoMemory::Lw => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xD0000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionTwoMemory::Sb => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xD4000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionTwoMemory::Sh => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xD8000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionTwoMemory::Sw => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *off {
                                segments.push32(
                                    current_section,
                                    0xDC000000
                                        | ((reg2 as u32) << 16)
                                        | ((reg1 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
            },
            ASTNode::InstructionThree {
                op,
                operand1: arg1,
                operand2: arg2,
                operand3: arg3,
            } => match op {
                InstructionThree::Addi => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x40000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Add => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x0 | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Sub => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x4000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Andi => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x44000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::And => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x8000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Ori => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x48000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Or => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0xC000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Xori => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x4C000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Xor => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x10000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Slli => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x50000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Sll => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x14000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Srli => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x54000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Srl => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x18000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Srai => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x54000400
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Sra => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x18000400
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Sltu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x20000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Sltiu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x5C000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Slti => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Immediate(val) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x58000000
                                        | ((reg1 as u32) << 16)
                                        | ((reg2 as u32) << 21)
                                        | (val & 0xFFFF),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Slt => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                segments.push32(
                                    current_section,
                                    0x1C000000
                                        | ((reg1 as u32) << 11)
                                        | ((reg2 as u32) << 16)
                                        | ((reg3 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Beq => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::LabelRef {
                                name: label,
                                is_local: _,
                            } = *arg3
                            {
                                segments.add_reloc_entry(
                                    current_section,
                                    label,
                                    RelocationType::Rel16,
                                );
                                segments.push32(
                                    current_section,
                                    0x8C000000 | ((reg1 as u32) << 16) | ((reg2 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Bne => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::LabelRef {
                                name: label,
                                is_local: _,
                            } = *arg3
                            {
                                segments.add_reloc_entry(
                                    current_section,
                                    label,
                                    RelocationType::Rel16,
                                );
                                segments.push32(
                                    current_section,
                                    0x90000000 | ((reg1 as u32) << 16) | ((reg2 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Bgeu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::LabelRef {
                                name: label,
                                is_local: _,
                            } = *arg3
                            {
                                segments.add_reloc_entry(
                                    current_section,
                                    label,
                                    RelocationType::Rel16,
                                );
                                segments.push32(
                                    current_section,
                                    0x9C000000 | ((reg1 as u32) << 16) | ((reg2 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Bltu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::LabelRef {
                                name: label,
                                is_local: _,
                            } = *arg3
                            {
                                segments.add_reloc_entry(
                                    current_section,
                                    label,
                                    RelocationType::Rel16,
                                );
                                segments.push32(
                                    current_section,
                                    0xA0000000 | ((reg1 as u32) << 16) | ((reg2 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Bge => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::LabelRef {
                                name: label,
                                is_local: _,
                            } = *arg3
                            {
                                segments.add_reloc_entry(
                                    current_section,
                                    label,
                                    RelocationType::Rel16,
                                );
                                segments.push32(
                                    current_section,
                                    0x94000000 | ((reg1 as u32) << 16) | ((reg2 as u32) << 21),
                                );
                            }
                        }
                    }
                }
                InstructionThree::Blt => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::LabelRef {
                                name: label,
                                is_local: _,
                            } = *arg3
                            {
                                segments.add_reloc_entry(
                                    current_section,
                                    label,
                                    RelocationType::Rel16,
                                );
                                segments.push32(
                                    current_section,
                                    0x98000000 | ((reg1 as u32) << 16) | ((reg2 as u32) << 21),
                                );
                            }
                        }
                    }
                }
            },
            ASTNode::InstructionFour {
                op,
                operand1: arg1,
                operand2: arg2,
                operand3: arg3,
                operand4: arg4,
            } => match op {
                InstructionFour::Mulu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                if let ASTNode::Register(reg4) = *arg4 {
                                    segments.push32(
                                        current_section,
                                        0x24000008
                                            | ((reg1 as u32) << 6)
                                            | ((reg2 as u32) << 11)
                                            | ((reg3 as u32) << 16)
                                            | ((reg4 as u32) << 21),
                                    );
                                }
                            }
                        }
                    }
                }
                InstructionFour::Mul => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                if let ASTNode::Register(reg4) = *arg4 {
                                    segments.push32(
                                        current_section,
                                        0x24000000
                                            | ((reg1 as u32) << 6)
                                            | ((reg2 as u32) << 11)
                                            | ((reg3 as u32) << 16)
                                            | ((reg4 as u32) << 21),
                                    );
                                }
                            }
                        }
                    }
                }
                InstructionFour::Divu => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                if let ASTNode::Register(reg4) = *arg4 {
                                    segments.push32(
                                        current_section,
                                        0x28000008
                                            | ((reg1 as u32) << 6)
                                            | ((reg2 as u32) << 11)
                                            | ((reg3 as u32) << 16)
                                            | ((reg4 as u32) << 21),
                                    );
                                }
                            }
                        }
                    }
                }
                InstructionFour::Div => {
                    if let ASTNode::Register(reg1) = *arg1 {
                        if let ASTNode::Register(reg2) = *arg2 {
                            if let ASTNode::Register(reg3) = *arg3 {
                                if let ASTNode::Register(reg4) = *arg4 {
                                    segments.push32(
                                        current_section,
                                        0x28000000
                                            | ((reg1 as u32) << 6)
                                            | ((reg2 as u32) << 11)
                                            | ((reg3 as u32) << 16)
                                            | ((reg4 as u32) << 21),
                                    );
                                }
                            }
                        }
                    }
                }
            },
            _ => {
                panic!("Unsupported AST Node: {:?}", node);
            }
        }
    }
    /* Now we need to relocate Rel16 branches */
    for (i, j) in segments.reloc.iter_mut() {
        for k in j.iter_mut().enumerate() {
            if k.1.reloc_type == RelocationType::Rel16 {
                let label_addr = segments.labels.get(i).unwrap().1;
                let cur_addr = if label_addr >= k.1.offset {
                    k.1.offset + 4
                } else {
                    k.1.offset + 8
                };
                let jump = (((label_addr.wrapping_sub(cur_addr)) as i32) / 4) as i16;
                segments.text[k.1.offset as usize] = ((jump as u16) & 0xFF) as u8;
                segments.text[(k.1.offset + 1) as usize] = (((jump as u16) & 0xFF00) >> 8) as u8;
            }
        }
        j.retain(|x| x.reloc_type != RelocationType::Rel16);
    }
    let mut data = Vec::new();
    data.extend_from_slice(b"\x89OkamiRO\x01\0\0\0\0\0\0\0");
    data.extend_from_slice((segments.text.len() as u32).to_le_bytes().as_slice());
    data.extend_from_slice((segments.rodata.len() as u32).to_le_bytes().as_slice());
    data.extend_from_slice((segments.data.len() as u32).to_le_bytes().as_slice());
    data.extend_from_slice(segments.bss.to_le_bytes().as_slice());
    let mut total_length = 0u32;
    for (_, i) in segments.reloc.iter() {
        total_length += (i.len() * 20) as u32;
    }
    data.extend_from_slice(total_length.to_le_bytes().as_slice());
    data.extend_from_slice(b"\0\0\0\0\0\0\0\0\0\0\0\0");
    data.extend_from_slice(segments.text.as_slice());
    if segments.text.len() % 4 != 0 {
        data.append(&mut vec![0u8; 4 - (segments.text.len() % 4)]);
    }
    data.extend_from_slice(segments.rodata.as_slice());
    if segments.rodata.len() % 4 != 0 {
        data.append(&mut vec![0u8; 4 - (segments.rodata.len() % 4)]);
    }
    data.extend_from_slice(segments.data.as_slice());
    if segments.data.len() % 4 != 0 {
        data.append(&mut vec![0u8; 4 - (segments.data.len() % 4)]);
    }
    for (i, j) in segments.reloc.iter_mut() {
        for k in j.iter_mut().enumerate() {
            let label = segments.labels.get(i).unwrap();
            data.extend_from_slice((k.1.reloc_type as u32).to_le_bytes().as_slice());
            data.extend_from_slice((k.1.section as u32).to_le_bytes().as_slice());
            data.extend_from_slice((label.0 as u32).to_le_bytes().as_slice());
            data.extend_from_slice(k.1.offset.to_le_bytes().as_slice());
            data.extend_from_slice(label.1.to_le_bytes().as_slice());
        }
    }
    std::fs::write(&args[2], data.as_slice()).expect("File write failed!");
    let end_time = std::time::Instant::now();
    println!(
        "Sucessfully compiled binary with size of {} bytes ({} secs elapsed)",
        data.len(),
        end_time
            .checked_duration_since(start_time)
            .unwrap()
            .as_secs_f32()
    );
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

fn splice_underscores(s: &str) -> String {
    String::from_iter(s.chars().filter(|c| *c != '_'))
}

fn to_ast_symbol(pair: pest::iterators::Pair<Rule>) -> ASTNode {
    match pair.as_rule() {
        Rule::assembly => to_ast_symbol(pair.into_inner().next().unwrap()),
        Rule::include_bin => {
            let inner = pair.into_inner().next().unwrap();
            match inner.as_rule() {
                Rule::imm_str => {
                    let s = inner.into_inner().next().unwrap().as_str();
                    return ASTNode::BinaryInclude(std::fs::read(s).unwrap());
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
            let inner = pair.into_inner().next().unwrap();
            return match inner.as_rule() {
                Rule::label_local_scope => {
                    let mut name = LAST_GLOBAL_LABEL.lock().unwrap().clone();
                    name.push('.');
                    name.push_str(inner.into_inner().next().unwrap().as_str());
                    return ASTNode::LabelDefine {
                        name,
                        is_local: true,
                        is_extern: false,
                        is_global: false,
                    };
                }
                Rule::label_global_scope => {
                    let in_in = &mut inner.into_inner();
                    let lbl_val1 = in_in.next().unwrap();
                    match lbl_val1.as_rule() {
                        Rule::label_kind => {
                            let lbl_val2 = in_in.next().unwrap();
                            let lbl_kind = lbl_val1.into_inner().next().unwrap().as_rule();
                            *LAST_GLOBAL_LABEL.lock().unwrap() = lbl_val2.as_str().to_string();
                            ASTNode::LabelDefine {
                                name: lbl_val2.as_str().to_string(),
                                is_local: false,
                                is_extern: lbl_kind == Rule::label_external,
                                is_global: lbl_kind == Rule::label_global,
                            }
                        }
                        Rule::label_name => {
                            *LAST_GLOBAL_LABEL.lock().unwrap() = lbl_val1.as_str().to_string();
                            return ASTNode::LabelDefine {
                                name: lbl_val1.as_str().to_string(),
                                is_local: false,
                                is_extern: false,
                                is_global: false,
                            };
                        }
                        _ => todo!(),
                    }
                }
                _ => todo!(),
            };
        }
        Rule::operand => {
            let val = pair.into_inner().next().unwrap();
            return match val.as_rule() {
                Rule::imm_bin => {
                    let num = u32::from_str_radix(
                        &splice_underscores(val.into_inner().next().unwrap().as_str()),
                        2,
                    )
                    .unwrap();
                    return ASTNode::Immediate(num);
                }
                Rule::imm_hex => {
                    let num = u32::from_str_radix(
                        &splice_underscores(val.into_inner().next().unwrap().as_str()),
                        16,
                    )
                    .unwrap();
                    return ASTNode::Immediate(num);
                }
                Rule::imm_dec => {
                    let num_str = &splice_underscores(val.into_inner().next().unwrap().as_str());
                    let num = i32::from_str_radix(num_str, 10)
                        .unwrap_or_else(|_| u32::from_str_radix(num_str, 10).unwrap() as i32);
                    return ASTNode::Immediate(num as u32);
                }
                Rule::imm_char => ASTNode::Immediate(
                    val.into_inner().next().unwrap().as_str().as_bytes()[0] as u32,
                ),
                Rule::label_name => {
                    if val.as_str().starts_with(".") {
                        let mut name = LAST_GLOBAL_LABEL.lock().unwrap().clone();
                        name.push_str(val.as_str());
                        ASTNode::LabelRef {
                            name: name,
                            is_local: true,
                        }
                    } else {
                        ASTNode::LabelRef {
                            name: val.as_str().to_string(),
                            is_local: false,
                        }
                    }
                }
                Rule::register => ASTNode::Register(match val.as_str() {
                    "zero" => 0,
                    "a0" => 1,
                    "a1" => 2,
                    "a2" => 3,
                    "a3" => 4,
                    "a4" => 5,
                    "a5" => 6,
                    "a6" => 7,
                    "a7" => 8,
                    "t0" => 9,
                    "t1" => 10,
                    "t2" => 11,
                    "t3" => 12,
                    "t4" => 13,
                    "t5" => 14,
                    "t6" => 15,
                    "t7" => 16,
                    "s0" => 17,
                    "s1" => 18,
                    "s2" => 19,
                    "s3" => 20,
                    "s4" => 21,
                    "s5" => 22,
                    "s6" => 23,
                    "s7" => 24,
                    "s8" => 25,
                    "s9" => 26,
                    "gp" => 27,
                    "tp" => 28,
                    "fp" => 29,
                    "sp" => 30,
                    "ra" => 31,
                    _ => todo!(),
                }),
                _ => todo!(),
            };
        }
        Rule::data => {
            let val = pair.into_inner().next().unwrap();
            match val.as_rule() {
                Rule::data_byte => {
                    return ASTNode::DataNum(
                        1,
                        Box::new(to_ast_symbol(val.into_inner().next().unwrap())),
                    );
                }
                Rule::data_half => {
                    return ASTNode::DataNum(
                        2,
                        Box::new(to_ast_symbol(val.into_inner().next().unwrap())),
                    );
                }
                Rule::data_word => {
                    return ASTNode::DataNum(
                        4,
                        Box::new(to_ast_symbol(val.into_inner().next().unwrap())),
                    );
                }
                Rule::data_str => {
                    let s = val
                        .into_inner()
                        .next()
                        .unwrap()
                        .into_inner()
                        .next()
                        .unwrap()
                        .as_str();
                    return ASTNode::DataString(String::from(s));
                }
                Rule::data_resb => {
                    if let ASTNode::Immediate(num) = to_ast_symbol(val.into_inner().next().unwrap())
                    {
                        return ASTNode::ExtendBSS(num);
                    } else {
                        panic!(".resb with non-integer immediate.");
                    }
                }
                Rule::data_resh => {
                    if let ASTNode::Immediate(num) = to_ast_symbol(val.into_inner().next().unwrap())
                    {
                        return ASTNode::ExtendBSS(num * 2);
                    } else {
                        panic!(".resh with non-integer immediate.");
                    }
                }
                Rule::data_resw => {
                    if let ASTNode::Immediate(num) = to_ast_symbol(val.into_inner().next().unwrap())
                    {
                        return ASTNode::ExtendBSS(num * 4);
                    } else {
                        panic!(".resw with non-integer immediate.");
                    }
                }
                _ => todo!(),
            }
        }
        _ => {
            panic!("UNKNOWN TOKEN: {}", pair.into_inner());
        }
    }
}
