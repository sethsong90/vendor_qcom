#! \Perl -w

print("\n*************************************************************************************************\n");
print("**************************** BMP review validation for DSNET QCM API ****************************\n");
print("*************************************************************************************************\n");
print("********* USAGE: perl dsnet_BMP_public.pl <path of //brewery/review/ on local machine> **********\n");
print("*************************************************************************************************\n");
print("** More info: Data Services QCM API presentation in DS eRoom                                   **\n");
print("** http://dmermprd02.qualcomm.com/eRoom/QCT-ProgramManagement_5/QCTDataServicesSoftware/0_4691 **\n");
print("*************************************************************************************************\n\n");

#open input file
open(INFILE,"dsnet_BMP_public.txt") or die("Can't open file dsnet_BMP_public.txt\n");

#read the entire input file content
@infile = <INFILE>;

chdir($ARGV[0]) or die("Can't cd to $ARGV[0] \n");
$cmd = "bash vdiff approved-inc ";

foreach my $line (@infile) { 
   $cmd = $cmd." ".$line;   
}

system($cmd);

print("\n************************************************************\n");
print("************************************************************\n");
print("************************************************************\n\n");
