{--------------------------------------------}
{ Pascal Compiler                            }
{ Copyright (C) 2023 TalonFox                }
{ Licensed Under the MIT License             }
{--------------------------------------------}
{
     NOTICE
     There are a few limitations to this Pascal Compiler, these include:
          - Some Standard Library functions may not be implemented depending on the OS (if using baremetal than there is no standard library)
          - The Real type is not supported (as well as any other floating-point number type)
          - You cannot pass or return records to and from functions and procedures (pointers to records are allowed though)
}

program OkamiPascal;

uses PascalFile, Lexer;

type
     PString = ^string;

procedure CompilerError(path: PString; line, col: integer; msg: string);
begin
     if path <> nil then
     begin
          WriteLn(#27'[1;31m',path^,'(',line,',',col,') - ',msg,#27'[0m');
     end else begin
          WriteLn(#27'[1;31mNoBuffer(?,?) - ',msg,#27'[0m');
     end;
     Halt;
end;

procedure HelpScreen;
begin
     WriteLn('Usage: OkamiPascal [target] [os] [input] [output]');
     WriteLn;
     WriteLn('Accepted Targets: okami1041');
     WriteLn('Accepted OSes: baremetal, fennec');
end;

var
     mainFile: PPascalSourceFile;
begin
     if ParamCount < 4 then HelpScreen else 
     begin
          mainFile := OpenSourceFile(ParamStr(3));
          writeLn('Compiling ',mainFile^.Name,'...');

     end;
end.