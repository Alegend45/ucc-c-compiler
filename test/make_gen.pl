#!/usr/bin/perl
use warnings;

# rules:
# "compiles"
# "compile-error"
# "exit=[0-9]+"
# "noop" - ignore
#
# empty implies "exit=0"

sub rules_parse;
sub rules_assume;
sub rules_exclude;
sub usage()
{
	die "Usage: $0\n";
}

usage() if @ARGV;

%rules = rules_exclude(rules_gendeps(rules_assume(rules_parse())));

print "EXES = " . join(' ', map {
		my $t = $rules{$_}->{target};

		$t . ($rules{$_}->{exit} ? ' ' . $t . '_TEST' : '')

	} keys %rules) . "\n";

print "test: \${EXES}\n";

print "clean:\n";
print "\trm -f \${EXES}\n";

for(keys %rules){
	print "$rules{$_}->{target}: $_\n";

	print "\t";

	my $fail_compile = $rules{$_}->{fail};

	if($fail_compile){
		print "! ";
	}

	print "../../ucc -w -o \$@ \$<\n";

	unless($fail_compile){
		my $ec = $rules{$_}->{exit};

		if(defined $ec){
			print "$rules{$_}->{target}_TEST: $rules{$_}->{target}\n";

			print "\t./\$<";
			if($ec){
				print "; [ \$\$? -eq $ec ]";
			}
		}

		print "\n";
	}
}

# -----------------------------------

sub lines
{
	my($f, $must_exist) = @_;
	open((my $fh), '<', $f) or ($must_exist ? die "open $f: $!\n" : return ());

	my @ret = <$fh>;
	close $fh;
	return map { chomp; $_ } @ret;
}

sub rule_new
{
	my $mode = shift;

	if($mode eq 'compiles'){
		return { };

	}elsif($mode eq 'compile-error'){
		return { fail => 1 }

	}elsif($mode =~ /^exit=([0-9]+)$/){
		return { 'exit' => $1 };

	}elsif($mode eq 'noop'){
		return { 'noop' => 1 };

	}else{
		die "bad rule \"$mode\"\n";
	}
}

sub rules_parse
{
	my %rules;

	for(lines('TestRules')){
		/(.*) *: *(.*)/ or die "bad rule: $_\n";
		my($ret, $fnames) = ($1, $2);

		my @fnames = split / *, */, $fnames;

		for(@fnames){
			die "$_ doesn't exist\n" unless -f $_;
			$rules{$_} = rule_new($ret);
		}
	}

	return %rules;
}

sub rules_assume
{
	my %rules = @_;
	for(glob '*.c'){
		unless($rules{$_}){
			# assume this file should succeed
			$rules{$_} = rule_new('exit=0');
		}
	}
	return %rules;
}

sub rules_gendeps
{
	my %rules = @_;
	for(keys %rules){
		($rules{$_}->{target} = $_) =~ s/\.c$//;
	}
	return %rules;
}

sub rules_exclude
{
	my %rules = @_;
	for(keys %rules){
		delete $rules{$_} if $rules{$_}->{noop};
	}
	return %rules;
}
