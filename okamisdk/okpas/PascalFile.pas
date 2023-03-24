unit PascalFile;
interface
    type
        PascalSourceFile = packed record
            Name: string;
            FileHandler: file of string;
            CurrentLineNum: integer;
            CurrentLine: string[255];
            CurrentCol: integer;
        end;
        PPascalSourceFile = ^PascalSourceFile;
    function OpenSourceFile(Name: string): PPascalSourceFile;
implementation
    function OpenSourceFile(Name: string): PPascalSourceFile;
    var
        srcIn: file of string;
        output: PPascalSourceFile;
    begin
        New(output);
        output^.Name := Name;
        output^.CurrentLineNum := 1;
        output^.CurrentLine := '';
        output^.CurrentCol := 0;
        Assign(srcIn,Name);
        output^.FileHandler := srcIn;
        OpenSourceFile := output;
    end;
end.