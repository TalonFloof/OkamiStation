type
    PascalSourceFile = record
        Name: string;
        Data: string;
        CurrentLine: integer;
        LineStart: integer;
        CurrentOffset: integer;
    end;
    PascalSourceFilePtr = ^PascalSourceFile;
function ReadSourceFile(Name: string): PascalSourceFile;
var
    srcIn: file of string;
    output: PascalSourceFile;
begin
    output.Name := Name;
    output.CurrentLine := 1;
    output.LineStart := 0;
    output.CurrentOffset := 0;
    Assign(srcIn,Name);
    Read(srcIn,output.Data);
    
    Close(srcIn);
    ReadSourceFile := output;
end;