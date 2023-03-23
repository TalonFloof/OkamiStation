{--------------------------------------------}
{ Pascal Compiler                            }
{ Copyright (C) 2023 TalonFox                }
{ Licensed Under the MIT License             }
{--------------------------------------------}
{
     NOTICE
     There are a few limitations to this Pascal Compiler, these include:
          - Lack of support for packed records (this is due to the alignment needs of RISC CPUs)
          - Some Standard Library functions may not be implemented depending on the OS (if using baremetal than there is no standard library)
          - The Real type is not supported (as well as any other floating-point number type)
          - 
}

program OkamiPascal;

type
     StringPtr = ^string;

procedure CompilerError(path: StringPtr; line, col: integer; msg: string);
begin
     if path = nil then
     begin
          WriteLn(#27'[1;31m',path^,'(',line,':',col,') - ',msg,#27'[0m');
     end else begin
          WriteLn(#27'[1;31mNoBuffer(?:?) - ',msg,#27'[0m');
     end;
     Halt;
end;

{$I PascalFile.pas}
{$I Parser.pas}

procedure HelpScreen;
begin
     WriteLn('Usage: OkamiPascal [options] [input] [output]');
     WriteLn;
     WriteLn('Options:');
     WriteLn('  -target=arch (REQUIRED)');
     WriteLn('      Set the target architecture to the given architecture');
     WriteLn('      (Accepted values: okami1041, xr17032. Sorry ry, fox32 will be supported later!)');
     WriteLn('  -os=os (REQUIRED)');
     WriteLn('      Set the target operating system to the given OS');
     WriteLn('      (Accepted values: fennec, mintia, baremetal)');
end;

var
     TargetArch: string[16];
     TargetOS: string[16];
     Temp: string;
begin
     if ParamCount < 4 then HelpScreen else 
     begin
          CompilerError(nil,0,0,'Error Test');
     end;
end.