# Practice4

---

## 练习1中所使用的添加路径使用understand库方法失效了

在practice1中使用的方法：


> 对软件目录中的understand.so使用stubgen生成.pyi文件，放在同一目录中，在脚本中添加路径：
>```python
>import sys
>sys.path.append('/Applications/Understand.app/Contents/MacOS/Python')
>import understand
>```

本次练习中使用此方法导致无法通过`funcs = db.ents(" ".join(["function", "~unknown", "~unresolved"]))`拿到函数（返回为空列表）

所以本次练习使用软件附带的upython解释器，其自带understand库。
`/Applications/Understand.app/Contents/MacOS/upython`

## NVDELT计算

### N

$N = N_1 + N_2$

$N_1 = $ total usage of all of the operators appearing in that implementation

$N_2 =$ total usage of all of the operands appearing in that implementation

### V

$V = Nlog_2 \mu$

用二进制编码时的程序长度，写长度为N的程序时，脑袋中比较的次数

$\mu = \mu_1 + \mu_2$

$\mu_1 =$ number of unique or distinct operators appearing in the implementation

$\mu_2 =$ number of unique or distinct operands appearing in that implementation


### D

$V^* =(2+\mu_2^*)log_2(2+\mu_2^*)$

$\mu_2^* =$ count intput + count_output

$D=1/L=V /V^*$

### L

$L = V^* / V$

### E

$E=V/L=VD$

### T

$T = E / S$ , $S=18$

## GetHalstedBaseMetrics(lexemes)函数

`main.py`中的`GetHalstedBaseMetrics(lexemes)`函数参考`c_misra_maint.pl`中的函数GetHalsteadBaseMetrics:
```perl
sub GetHalsteadBaseMetrics {
    my ($lexer,$startLine,$endLine) = @_;
    my $n1=0;
    my $n2=0;
    my $N1=0;
    my $N2=0;
    
    my %n1 = ();
    my %n2 = ();
    
    foreach my $lexeme ($lexer->lexemes($startLine,$endLine)) {
        if(($lexeme->token eq "Operator") ||
                ($lexeme->token eq "Keyword") ||
                ($lexeme->token eq "Punctuation")) {  
            if($lexeme->text() !~ /[)}\]]/) {
                $n1{$lexeme->text()} = 1;
                $N1++;
            } # end if($lexeme->text() !~ /[)}\]]/)
        }elsif(($lexeme->token eq "Identifier") ||
                ($lexeme->token eq "Literal") || ($lexeme->token eq "String")){
            $n2{$lexeme->text()} = 1;
            $N2++;
        } # end if(...)
    } # end foreach my $lexeme ($lexer->lexemes($startLine,$endLine))
    
    $n1 = scalar(keys(%n1));
    $n2 = scalar(keys(%n2));  
    return ($n1,$n2,$N1,$N2);
} # end sub GetHalsteadBaseMetrics ()
```

