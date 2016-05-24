#------------------------------------------------------------------
# Description: This file is to automate the test suite execution
#------------------------------------------------------------------
#------------------------------------------------------------------
# Copyright (c) 2010 Qualcomm Technologies, Inc.
# All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
#------------------------------------------------------------------
#------------------------------------------------------------------
#            EDIT HISTORY FOR FILE
#    When      Who   What
#   ======    ===== ======
# 12/29/2009    kr   Initial Version
#------------------------------------------------------------------

use Getopt::Long;

my %config = (
	      verbose     => 0,
	      suite       => undef,
	      test        => undef,
              random      => 0,
              auto        => 0,
	      loop        => 1,
	      duration    => 0,
              help        => 0,
	     );

my @opt_defs = ('verbose', 'suite=s@', 'test=s@', 'random', 'auto', 'loop=i', 'duration=i', 'help');

my @rpc_tests = ('ping_mdm_clnt_test_0000',
                 'ping_mdm_clnt_test_0001',
                 'ping_mdm_clnt_test_0002',
                 'ping_mdm_clnt_test_1000',
                 'ping_mdm_clnt_test_1001',
                 'ping_mdm_clnt_test_1002',
                 'librpc_oncrpc_nv_test',
                 'librpc_ping_mdm_test_0000',
                 'librpc_cb_test',
                 'librpc_test',
                 'librpc_ping_mdm_test_0001',
#                 'oncrpc_test_threads_mid',
#                 'oncrpc_test_threads_multi',
                 'oncrpc_test_threads',
#                 'oncrpc_test',
                 'oncrpc_test_cm_0001',
                 'oncrpc_test_snd_0001',
                 'oncrpc_test_snd_0002');

my @modem_api_tests = ();

my @kernel_tests = ('smd_tty_loopback_test',
                    'smem_log_test',
                    'ping_null_test',
                    'ping_reg_test',
                    'ping_data_reg_test',
                    'ping_data_cb_reg_test',
                    'oem_rapi_null',
                    'oem_rapi_streaming_func');

my @remote_api_tests = ('remote_apis_verify');

my %fail_count = ();

my %navlbl_count = ();

my %pass_with_fail_msg_count = ();

my $log_file = undef;

my $loop_count = 0;

my $line = "="x80;

main();

sub main() {

  #get options
  my $opt_result = GetOptions(\%config, @opt_defs);
  if (!$opt_result || $config{help} ||
      ($config{loop} < 0) || ($config{duration} < 0)) {
    display_usage();
    return;
  }

  my $cur_time = time();
  ($sec,$min,$hr,$dd,$mm,$yyyy,$wday,$yday,$isdst) = localtime($cur_time);
  $yyyy += 1900;
  $mm += 1;
  my $file_name = "log-$hr.$min.$sec-$mm.$dd.$yyyy";
  print "$file_name\n";

  open(LOG_FILE,"> $file_name") or die "Can't open: $file_name\n";
  $log_file = *LOG_FILE;
  printf $log_file "%-80s\n",$line;
  printf $log_file "%-30s %12s %12s %12s\n","Test Name","Loop #","# of Fails","Status";
  printf $log_file "%-80s\n",$line;

  #mount debugfs
  $result = system("adb shell \"mkdir /data/debug2\"");
  if (!($result >>=8)) {
    print "Mounting debugfs\n";
    $result = system("adb shell \"mount -t debugfs debugfs /data/debug2\"");
    if (($result >>=8) != 0) {
      print "mounting debugfs failed\n";
      return;
    }
  }
  else {
      print "can't create mount point for debugfs\n";
      return
  }

  if ($config{auto}) {
    $result = `adb shell ls /data/rpc-tests`;
    @rpc_tests = split(/[\n|\r]+/, $result);
    $result = `adb shell ls /data/modem-api-test`;
    @modem_api_tests = split(/[\n|\r]+/, $result);
  }

  my $init_time = time();

  if (!(defined @{$config{suite}}) && !(defined @{$config{test}})) {
    @{$config{suite}} = ("all");
  }

  #run tests for requested iterations
  while(1) {

    if ($config{duration} && ($config{loop} eq 1)) {
	$config{loop} = 0;
    }

    $diff = time() - $init_time;
    if ($config{duration} && (($diff / 60) > $config{duration})) {
      printf "Time limit reached\n";
      last;
    }

    $loop_count++;
    printf "Loop count = %d\n", $loop_count;

    foreach $suite (@{$config{suite}}) {

      if ($suite eq "rpc") {
        handle_rpc_tests();
      }
      elsif ($suite eq "modem_api") {
        handle_modem_api_tests();
      }
      elsif ($suite eq "kernel") {
        handle_kernel_tests();
      }
      elsif ($suite eq "remote_api") {
        handle_remote_api_tests();
      }
      elsif ($suite eq "all") {
        handle_rpc_tests();
        handle_modem_api_tests();
        handle_kernel_tests();
        handle_remote_api_tests();
      }
      else {
        display_usage();
        return;
      }
    }

    if (defined @{$config{test}}) {
      print "\n";
      handle_rpc_tests(@{$config{test}});
      handle_modem_api_tests(@{$config{test}});
      handle_kernel_tests(@{$config{test}});
      handle_remote_api_tests(@{$config{test}});
    }

    printf "\nTest time = %d sec\n", time() - $init_time;
    if($config{loop}) {
      if ($loop_count >= $config{loop}) {
        last;
      }
    }
  }
  printf "\nTest duration = %d sec\n", time() - $init_time;
  printf $log_file "\n%-80s\n",$line;
  print $log_file "Summary\n";
  printf $log_file "%-30s %12s %12s %12s\n","Test Name","# of Loops","# of Fails","Status";
  printf $log_file "%-80s\n",$line;
  foreach $key (sort keys %fail_count) {
    $overall_status = $pass_with_fail_msg_count{$key}?"PASS+FAIL":"PASSED";
    $overall_status = $navlbl_count{$key}?"Not Avail.":$overall_status;
    $overall_status = $fail_count{$key}?"FAILED":$overall_status;
    printf $log_file "%-30s %12d %12d %12s\n",$key,$loop_count,$fail_count{$key},$overall_status;
  }
  close(LOG_FILE);

  printf "\n%-80s",$line;
  print "Summary\n";
  printf "%-30s %12s %12s %12s\n","Test Name","# of Loops","# of Fails","Status";
  printf "%-80s\n",$line;
  foreach $key (sort keys %fail_count) {
    $overall_status = $pass_with_fail_msg_count{$key}?"PASS"+FAIL:"PASSED";
    $overall_status = $navlbl_count{$key}?"Not Avail.":$overall_status;
    $overall_status = $fail_count{$key}?"FAILED":$overall_status;
    printf "%-30s %12d %12d %12s\n",$key,$loop_count,$fail_count{$key},$overall_status;
  }
}

sub display_usage() {
  print "Usage: test.pl [OPTION]\n";
  print "OPTION:\n";
  print "-h, --help              Prints this help message\n";
  print "-v, --verbose           Turns on verbose mode. Test output\n";
  print "                        will be dumped on the screen\n";
  print "-t, --test=TEST         Runs the test TEST\n";
  print "                        Following tests are available\n";
  print "                           rpc tests\n";
  foreach (@rpc_tests) {
    print "                              $_\n";
  }
  print "                           kernel tests\n";
  foreach (@kernel_tests) {
    print "                              $_\n";
  }
  print "                           remote_api tests\n";
  foreach (@remote_api_tests) {
    print "                              $_\n";
  }
  print "                        Multiple tests can be specified.\n";
  print "-s, --suite=SUITE       Runs all the tests in suite SUITE.\n";
  print "                        The SUITE can be one of the following\n";
  print "                           rpc\n";
  print "                           kernel\n";
  print "                           remote_api\n";
  print "                        If 'all' is specified, all the above suites\n";
  print "                        will be executed.\n";
  print "-l, --loop=LOOP_COUNT   Runs the specified tests LOOP_COUNT number\n";
  print "                        of times.  Default value is 1.  If 0 is\n";
  print "                        specified, tests will run for ever, or for\n";
  print "                        a time duration if option 'duration' is \n";
  print "                        specified. LOOP_COUNT should be a +ve value.\n";
  print "-d, --duration=DURATION Runs the specified tests in loop for a\n";
  print "                        duration of DURATION minutes. If not\n";
  print "                        specified, tests will run indefinitely or\n";
  print "                        for the specified number of loops.\n";
  print "                        DURATION should be a +ve value.\n";
  print "-r, --random            Randomize the test sequence.\n";
  print "-a, --auto              Auto construct rpc & modem_api test suites.\n\n";
  print "To run tests indefinitely\n";
  print "specify '-l = 0' and don't use -d (i.e., '-d = 0')\n";
}

sub randomize {

  if (scalar(@_) eq 0) {
    print "No elements to randomize\n";
    return;
  }
  else {
    @test_array = @_;
  }

  my $test_size = @test_array;

  for($i = ($test_size - 1); $i >= 0; $i--) {
    $j = int(rand($test_size - 1));
    $temp = $test_array[$j];
    $test_array[$j] = $test_array[$i];
    $test_array[$i] = $temp;
  }
  print "\n";

  return @test_array;
}

sub handle_rpc_tests() {

  #decide tests to run
  if (scalar(@_) eq 0) {
    @tests = @rpc_tests;
    print "\nRunning rpc tests\n\n";
  }
  else {
    @tests = @_;
  }

  if($config{random}) {
    @tests = randomize(@tests);
  }

  foreach $test (@tests) {

    #check if test exists
    if (!grep(/^$test$/, @rpc_tests) ) {
      next;
    }

    if(!exists($fail_count{$test}))
    {
      $fail_count{$test} = 0;
    }
    if(!exists($navlbl_count{$test}))
    {
      $navlbl_count{$test} = 0;
    }
    if(!exists($pass_with_fail_msg_count{$test}))
    {
      $pass_with_fail_msg_count{$test} = 0;
    }

    print "Running test $test ...\n";
    $result = system("adb shell chmod 755 /data/rpc-tests/$test");

    if ($config{verbose}) {
      $result = system("adb shell /data/rpc-tests/$test");
    }
    else {
      $result = `adb shell /data/rpc-tests/$test`;

      if($result !~  m/[\n|\r]*\s*(FAIL|PASS)\s*[\n|\r]*/) {
        if ($result =~ m/not found/) {
          $navlbl_count{$test}++;
          $status = "NOT AVAILABLE";
        }
        else {
          print "FAILED -- Unable to determine PASS or FAIL \n";
          $status = "FAILED";
        }
      }
      else
      {
        if($result =~ m/[\n|\r]*\s*FAIL\s*[\n|\r]*/)
        {
          $status = "FAILED";
          $fail_count{$test}++;
          printf $log_file "%s: %-40s\n",$test,$result;
          printf "%-80s\n",$line;
          print "FAILED -- Test returns failure \n";
          print "Failure details\n $result \n";
          printf "%-80s\n",$line;
        }
        elsif($result =~ m/[\n|\r]*\s*PASS\s*[\n|\r]*/)
        {
          $status = "PASSED";
          if ($result =~ m/FAIL/) {
            $status = "PASS+FAIL";
            $pass_with_fail_msg_count{$test}++;
            printf "%-80s\n",$line;
            print "Some warning during test, final result reported is PASS \n";
            print "Warning Details\n $result \n";
            printf "%-80s\n",$line;
          }
        }
      }
      print "$status\n";
      printf $log_file "%-30s %12d %12d %12s\n",$test,$loop_count,$fail_count{$test},$status;
    }
  }
}

sub handle_modem_api_tests() {

  if (scalar(@_) eq 0) {
    @tests = @modem_api_tests;
    print "\nRunning modem api tests\n\n";
  }
  else {
    @tests = @_;
  }

  if ($config{random}) {
    @tests = randomize(@tests);
  }

  foreach $test (@tests) {

    #check if test exists
    if (!grep(/^$test$/, @modem_api_tests) ) {
      next;
    }

    if(!exists($fail_count{$test}))
    {
      $fail_count{$test} = 0;
    }
    if(!exists($navlbl_count{$test}))
    {
      $navlbl_count{$test} = 0;
    }
    if(!exists($pass_with_fail_msg_count{$test}))
    {
      $pass_with_fail_msg_count{$test} = 0;
    }

    print "Running test $test ...\n";
    $result = system("adb shell chmod 755 /data/modem-api-test/$test");

    if ($config{verbose}) {
      $result = system("adb shell /data/modem-api-test/$test");
    }
    else {
      $result = `adb shell /data/modem-api-test/$test`;

      if($result !~  m/[\n|\r]*\s*(FAIL|PASS)\s*[\n|\r]*/) {
        if ($result =~ m/not found/) {
          $navlbl_count{$test}++;
          $status = "NOT AVAILABLE";
        }
        else {
          print "FAILED -- Unable to determine PASS or FAIL \n";
          $status = "FAILED";
        }
      }
      else
      {
        if($result =~ m/[\n|\r]*\s*FAIL\s*[\n|\r]*/)
        {
          $status = "FAILED";
          $fail_count{$test}++;
          printf $log_file "%s: %-40s\n",$test,$result;
          printf "%-80s\n",$line;
          print "FAILED -- Test returns failure \n";
          print "Failure details\n $result \n";
          printf "%-80s\n",$line;
        }
        elsif($result =~ m/[\n|\r]*\s*PASS\s*[\n|\r]*/)
        {
          #print "PASSED \n";
          $status = "PASSED";
          if ($result =~ m/FAIL/) {
            $status = "PASS+FAIL";
            $pass_with_fail_msg_count{$test}++;
            printf "%-80s\n",$line;
            print "Some warning during test, final result reported is PASS \n";
            print "Warning Details\n $result \n";
            printf "%-80s\n",$line;
          }
        }
      }
      print "$status\n";
      printf $log_file "%-30s %12d %12d %12s\n",$test,$loop_count,$fail_count{$test},$status;
    }
  }
}

sub handle_kernel_tests() {

  #decide tests to run
  if (scalar(@_) eq 0) {
    @tests = @kernel_tests;
    print "\nRunning kernel tests\n\n";
  }
  else {
    @tests = @_;
  }

  if ($config{random}) {
    @tests = randomize(@tests);
  }

  foreach $test (@tests) {

    #check if test is valid
    if (!grep(/^$test$/, @kernel_tests) ) {
      next;
    }

    if(!exists($fail_count{$test}))
    {
      $fail_count{$test} = 0;
    }
    if(!exists($navlbl_count{$test}))
    {
      $navlbl_count{$test} = 0;
    }
    if(!exists($pass_with_fail_msg_count{$test}))
    {
      $pass_with_fail_msg_count{$test} = 0;
    }

    print "Running test $test ...\n";

    if ($test =~ m/^ping_(.*)/) {
      #ping tests
      $result = system("adb shell \"echo $1 > /data/debug2/ping_mdm\"");
      $result = int(`adb shell cat /data/debug2/ping_mdm`);
      if ($result) {
        $fail_count{$test}++;
        $status = "FAILED";
        printf $log_file "%s: %-40s\n",$test,$result;
      }
      else {
        $status = "PASSED";
      }
    }
    elsif ($test =~ m/^oem_rapi_(.*)/) {
      #oem rapi tests
      $result = system("adb shell \"echo $1 > /data/debug2/oem_rapi\"");
      $result = int(`adb shell cat /data/debug2/oem_rapi`);
      if ($result) {
        $fail_count{$test}++;
        $status = "FAILED";
        printf $log_file "%s: %-40s\n",$test,$result;
      }
      else {
        $status = "PASSED";
      }
    }
    else {
      #other executable tests
      $result = system("adb shell chmod 755 /data/kernel-tests/$test");
      if ($config{verbose}) {
        $result = system("adb shell /data/kernel-tests/$test");
      }
      else {
        $result = `adb shell /data/kernel-tests/$test`;
        if($result !~  m/[\n|\r]*\s*(FAIL|PASS)\s*[\n|\r]*/) {
          if ($result =~ m/not found/) {
            $navlbl_count{$test}++;
            $status = "NOT AVAILABLE";
          }
          else {
            print "FAILED -- Unable to determine PASS or FAIL \n";
            $status = "FAILED";
          }
        }
        else
        {
          if($result =~ m/[\n|\r]*\s*FAIL\s*[\n|\r]*/)
          {
            $status = "FAILED";
            $fail_count{$test}++;
            printf $log_file "%s: %-40s\n",$test,$result;
            printf "%-80s\n",$line;
            print "FAILED -- Test returns failure \n";
            print "Failure details\n $result \n";
            printf "%-80s\n",$line;
          }
          elsif($result =~ m/[\n|\r]*\s*PASS\s*[\n|\r]*/)
          {
            #print "PASSED \n";
            $status = "PASSED";
            if ($result =~ m/FAIL/) {
              $status = "PASS+FAIL";
              $pass_with_fail_msg_count{$test}++;
              printf "%-80s\n",$line;
              print "Some warning during test, final result reported is PASS \n";
              print "Warning Details\n $result \n";
              printf "%-80s\n",$line;
            }
          }
        }
      }
    }
    print "$status\n";
    printf $log_file "%-30s %12d %12d %12s\n",$test,$loop_count,$fail_count{$test},$status;
  }
}

sub handle_remote_api_tests() {

  #decide tests to run
  if (scalar(@_) eq 0) {
    @tests = @remote_api_tests;
    print "\nRunning remote api tests\n\n";
  }
  else {
    @tests = @_;
  }

  if ($config{random}) {
    @tests = randomize(@tests);
  }

  foreach $test (@tests) {

    #check if test is valid
    if (!grep(/^$test$/, @remote_api_tests) ) {
      next;
    }

    if(!exists($fail_count{$test})) {
      $fail_count{$test} = 0;
    }
    if(!exists($navlbl_count{$test})) {
      $navlbl_count{$test} = 0;
    }
    if(!exists($pass_with_fail_msg_count{$test}))
    {
      $pass_with_fail_msg_count{$test} = 0;
    }


    print "Running test $test ...\n";
    my $test_path = "/data/remote-api-tests/oncrpc/";
    $result = system("adb shell chmod 755 $test_path$test");
    if ($config{verbose}) {
      $result = system("adb shell $test_path$test");
    }
    else {
      $result = `adb shell $test_path$test`;
      if($result !~  m/[\n|\r]*\s*(FAIL|PASS)\s*[\n|\r]*/) {
        if ($result =~ m/not found/) {
          $navlbl_count{$test}++;
          $status = "NOT AVAILABLE";
        }
        else {
          print "FAILED -- Unable to determine PASS or FAIL \n";
          $status = "FAILED";
        }
      }
      else
      {
        if($result =~ m/[\n|\r]*\s*FAIL\s*[\n|\r]*/)
        {
          $status = "FAILED";
          $fail_count{$test}++;
          printf $log_file "%s: %-40s\n",$test,$result;
          printf "%-80s\n",$line;
          print "FAILED -- Test returns failure \n";
          print "Failure details\n $result \n";
          printf "%-80s\n",$line;
        }
        elsif($result =~ m/[\n|\r]*\s*PASS\s*[\n|\r]*/)
        {
          #print "PASSED \n";
          $status = "PASSED";
          if ($result =~ m/FAIL/) {
            $status = "PASS+FAIL";
            $pass_with_fail_msg_count{$test}++;
            printf "%-80s\n",$line;
            print "Some warning during test, final result reported is PASS \n";
            print "Warning Details\n $result \n";
            printf "%-80s\n",$line;
          }
        }
      }
      print "$status\n";
      printf $log_file "%-30s %12d %12d %12s\n",$test,$loop_count,$fail_count{$test},$status;
    }
  }
}
