{
    Pascal Precedence (sorted from highest to lowest)
    Unary Operators       | @, not
    Multiplying Operators | *, /, div, mod, and, shl, shr
    Adding Operators      | +, -, or, xor
    Relational Operators  | =, <>, <, >, <=, >=, in
}
unit Lexer;
interface
    uses PascalFile;
    type
        TokenKind = (
            tkNil,
            tkIdentifier,
            tkSymbol,
            tkKeyword,
            tkString,
            tkNumber
        );
        Token = packed record
            tkType: TokenKind;
            source: PPascalSourceFile;
            line: integer;
            col: integer;
            data: string[255];
        end;
        PToken = ^Token;
    function NextToken(src: PPascalSourceFile): PToken;
implementation
    function IsAlpha(Chr : char) : boolean;
    begin
        IsAlpha := Chr in ['a'..'z', 'A'..'Z']
    end;

    function IsDigit(Chr : char) : boolean;
    begin
        IsDigit := Chr in ['0'..'9']
    end;

    function IsHexDigit(Chr : char) : boolean;
    begin
        IsHexDigit := Chr in ['0'..'9', 'a'..'f', 'A'..'F']
    end;

    function IsAlphaNum(Chr : char) : boolean;
    begin
        IsAlphaNum := IsAlpha(Chr) or IsDigit(Chr)
    end;

    function PeekChar(src: PPascalSourceFile): char;
    var
        c: char;
    begin
        
    end;
    function NextChar(src: PPascalSourceFile): char;
    var
        c: char;
    begin
        if src^.CurrentCol >= Length(src^.CurrentLine) then begin
            src^.CurrentCol := 0;
            src^.CurrentLineNum := src^.CurrentLineNum + 1;
            readLn(File,src^.CurrentLine);
        end else begin

            src^.CurrentCol := src^.CurrentCol + 1;
        end;
    end;

    function NextToken(src: PPascalSourceFile): PToken;
    var
        c: char;
        tok: PToken;
    begin
        New(tok);
        with tok^ do
        begin
            tkType := tkNil;
            source := src;
            c := NextChar();
            if IsDigit(c) then
            begin

            end;

        end;
        NextToken := tok;
    end;
end.