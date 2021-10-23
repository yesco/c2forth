$f = join("", <>);
$f =~ s/\s+/ /smg;

@strings= ();
$snum= 0;

$f =~ s/("([^"]|\.)*")/($strings[$snum++]=$1, "\"$snum\"")/eg;

$self = "[!\\\"#\$%&\'\(\)\*\+,\-./0123456789:;<=>?@\[\\\]\^_\`abcdefghijklmnopqrstuvwxyz\{\|\}~]";

%words = (
    "if", "?(",
    "exit", "]",
    "unloop", "]",
    "drop", "\\",

    "parloc", "l#",
    "setlocals", "l,",
    "local@", "l@",
    "local!", "l!",

    ":=", "`",
    "==", "=",
    "endif", ")",

    # import functions
    # TODO: add par count in compilation

    "putchar", "14 x",
    "printf", "16 x",
);

#$wnum= 128+65; # 'A'...

$wnum= 65; # 'A'...

sub w {
    my ($w)= @_;
    $w =~ s/^_(\S+)$/$1/;
    my $r= $words{$w};
    return $r if $r;
    return $w if $w =~ /^[^a-zA-Z]*$/;

    if ($wnum>=255) {
        die "problem- too many wnum!";
    }

    $r= chr($wnum++);
    $words{$w} = $r;
    $words{$r} = $w; # yeah... lol

    # TODO: local vars
    print STDERR "\nDEFINED $r = $w\n";

    return $r;
}

$f =~ s/(\S+)/w($1)/smeg;
$f =~ s/^\s*//smg;
$f =~ s/\s*$//smg;

# remove most spaces
while ($f =~ s/([^\d])\s+(.)/$1$2/smg) {};
while ($f =~ s/([^\d])\s+(\d)/$1$2/smg) {};
while ($f =~ s/(\d)\s+([^\d])/$1$2/smg) {};

# restore strings
$f =~ s/"(\d+)"/$strings[$1-1]/smeg;

print "$f\n";

