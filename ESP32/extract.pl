while (<>) {
    chop();
    next if /^\/\*/;
    next if /^\*\//;
    next if /^\s+/;
    next if /^$/;
    s/PROVIDE \( (.*) \);/$1/;
    next if /^_/;

    my $name;
    if (/^([a-z]\w+)\s+=/i) {
        $name= $1;
        printf "$name\n";
    } else {
        printf STDERR "%% ??? $_\n";
    }
}
