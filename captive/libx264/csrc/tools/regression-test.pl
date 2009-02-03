#!/bin/env perl

# regression-test.pl: tests different versions of x264
# by Alex Izvorski & Loren Merrit 2007
# GPL

$^W=1;

use Getopt::Long;
use File::Path;
use File::Copy;
use File::Basename;

# prerequisites: 
# - perl > 5.8.0
# - svn
# - net access
# - linux/unix
# - gcc

# the following require a make testclean
# - changing x264 option sets and or adding/removing option sets
# - changing configure options
# - changing CFLAGS or other variables that affect compilation
# - a newer head revision

# the following do not require a make testclean, but may cause some tests to be rerun unnecessarily:
# - adding/removing input files 
# - adding/removing versions

@versions = ();
@input_files = ();
@option_sets = ();

GetOptions ("version=s" => \@versions,
            "input=s" => \@input_files,
            "options=s" => \@option_sets,
            );

# TODO check inputs
# TODO some way to give make options
# TODO some way to give configure options

if (scalar(@versions) == 0)
{
    @versions = ('rHEAD', 'current');
}

if (scalar(@option_sets) == 0 ||
    scalar(@input_files) == 0)
{
    print "Regression test for x264\n";
    print "Usage:\n  perl tools/regression-test.pl --version=623 --version=624 --input=football_720x480p.yuv --input=foreman_352x288p.yuv --options='--me=dia --subme=2 --no-cabac'\n";
    print "Any number of versions, option sets, and input files may be given.\n";
    print "Versions may be any svn revision, a comma-separated list of revisions, or 'current' for the version in the current directory.\n";
    exit;
}

mkpath("test");

@versions = map { split m![,\s]\s*! } @versions;

foreach my $version (@versions)
{
    $version =~ s!^head$!rHEAD!i;
    $version =~ s!^current$!current!i;
    $version =~ s!^(\d+)$!r$1!;

    if (-e "test/x264-$version/x264" && ($version ne "current"))
    {
        print("have version: $version\n");
        next;
    }
    print("building version: $version\n");
    if ($version eq "current")
    {
        system("(./configure && make) &> test/build.log");
        mkpath("test/x264-$version");
        if (! -e "x264") { print("build failed \n"); exit 1; }
        copy("x264", "test/x264-$version/x264");
        chmod(0755, "test/x264-$version/x264");
        next;
    }
    system("svn checkout -$version svn://svn.videolan.org/x264/trunk/ test/x264-$version >/dev/null");
    chdir("test/x264-$version");
    system("(./configure && make) &> build.log");
    chdir("../..");
    if (! -e "test/x264-$version/x264") { print("build failed \n"); exit 1; }
}

$any_diff = 0;
foreach my $i (0 .. scalar(@option_sets)-1)
{
    $opt = $option_sets[$i];
    print("options: $opt \n");
    foreach my $j (0 .. scalar(@input_files)-1)
    {
        my $file = $input_files[$j];
        print("input file: $file \n");

        my $outfile = basename($file);
        $outfile =~ s!\.yuv$!!;
        $outfile = "test/opt$i-$outfile$j";

        foreach my $k (0 .. scalar(@versions)-1)
        {
            my $version = $versions[$k];

            if (-e "$outfile-$version.log" && ($version ne "current"))
            {
                print("have results for version: $version\n");
            }
            else
            {
                print("running version: $version \n");
                # verbose option is required for frame-by-frame comparison
                system("test/x264-$version/x264 --verbose $opt $file -o $outfile-$version.264 > $outfile-$version.log 2>&1");
            }

            # TODO check for crashes
            # TODO if (read_file("$outfile-$version.log") =~  m!could not open input file!) ...
            # TODO check decompression with jm
            # TODO dump (and check) frames

            if ($k > 0)
            {
                my $baseversion = $versions[$k - 1];
                print("comparing $version with $baseversion: ");
                $is_diff = 0;
                $is_diff ||= compare_logs("$outfile-$version.log", "$outfile-$baseversion.log");
                $is_diff ||= compare_raw264("$outfile-$version.264", "$outfile-$baseversion.264");
                if (! $is_diff) { print("identical \n"); }
                $any_diff ||= $is_diff;
            }
        }
    }
}

print "\n";
if (! $any_diff) { print "no differences found\n"; }
else { print "some differences found\n"; exit 1; }


sub compare_logs
{
    my ($log1, $log2) = @_;

    my $logdata1 = read_file($log1);
    my $logdata2 = read_file($log2);

    # FIXME comparing versions with different log output format will fail
    $logdata1 = join("\n", grep { m!frame=! } split(m!\n!, $logdata1));
    $logdata2 = join("\n", grep { m!frame=! } split(m!\n!, $logdata2));

    my $is_diff = 0;
    if ($logdata1 ne $logdata2)
    {
        print("log files differ \n");
        $is_diff = 1;
    }

    my $stats1 = parse_log_overall_stats($log1);
    my $stats2 = parse_log_overall_stats($log2);

    if ($stats1->{psnr_y} != $stats2->{psnr_y})
    {
        printf("psnr change: %+f dB \n", $stats1->{psnr_y} - $stats2->{psnr_y});
        $is_diff = 1;
    }
    if ($stats1->{bitrate} != $stats2->{bitrate})
    {
        printf("bitrate change: %+f kb/s \n", $stats1->{bitrate} - $stats2->{bitrate});
        $is_diff = 1;
    }

    #arbitrarily set cutoff to 3% change
    #$speed_change_min = 0.03;
    #if (abs($stats1->{fps} - $stats2->{fps}) > $speed_change_min * ($stats1->{fps} + $stats2->{fps})/2)
    #{
    #    printf("speed change: %+f fps \n", $stats1->{fps} - $stats2->{fps}); 
    #    $is_diff = 1;
    #}

    return $is_diff;

    # TODO compare frame by frame PSNR/SSIM, record improved or unimproved ranges
    #parse_log_frame_stats($log1);
    #parse_log_frame_stats($log2);

    # TODO compare actual run times
}

sub compare_raw264
{
    my ($raw1, $raw2) = @_;
    
    # FIXME this may use a lot of memory
    my $rawdata1 = read_file($raw1);
    my $rawdata2 = read_file($raw2);
    
    # remove first NAL, it is a version-specific SEI NAL
    my $pat = chr(0).chr(0).chr(1);
    $rawdata1 =~ s!^.*?$pat.*?$pat!$pat!;
    $rawdata2 =~ s!^.*?$pat.*?$pat!$pat!;

    if ($rawdata1 ne $rawdata2)
    {
        print("compressed files differ \n");
        return 1;
    }

    return 0;
}

sub parse_log_frame_stats
{
    my ($log) = @_;
    my $logtext = read_file($log);

    my @frames = ();
    while ($logtext =~ m!x264 \[debug]: (frame=.*)!g)
    {
        my $line = $1;
        if ($line !~ m!frame=\s*(\d+)\s* QP=\s*(\d+)\s* NAL=(\d+)\s* Slice:(\w)\s* Poc:(\d+)\s* I:(\d+)\s* P:(\d+)\s* SKIP:(\d+)\s* size=(\d+) bytes PSNR Y:(\d+\.\d+) U:(\d+\.\d+) V:(\d+\.\d+)!)
        {
            print "error: unparseable log line: $line \n"; next;
        }
        
        my $frame = 
            +{
                num=>$1,
                qp=>$2,
                nal=>$3,
                slice=>$4,
                poc=>$5,
                count_i=>$6,
                count_p=>$7,
                count_skip=>$8,
                size=>$9,
                psnr_y=>$10,
                psnr_u=>$11,
                psnr_v=>$12,
            };

        if ($line =~ m!SSIM Y:(\d+\.\d+)!)
        {
            $frame->{ssim} = $1;
        }

        push(@frames, $frame);
    }

    return @frames;
}

sub parse_log_overall_stats
{
    my ($log) = @_;
    my $logtext = read_file($log);

    if ($logtext !~ m!x264 \[info\]: PSNR Mean Y:(\d+\.\d+) U:(\d+\.\d+) V:(\d+\.\d+) Avg:(\d+\.\d+) Global:(\d+\.\d+) kb/s:(\d+\.\d+)!)
    {
        print "error: unparseable log summary info \n"; return +{};
    }
    my $stats = 
        +{ 
            psnr_y=>$1,
            psnr_u=>$2,
            psnr_v=>$3,
            psnr_avg=>$4,
            psnr_global=>$5,
            bitrate=>$6,
        };
    
    if ($logtext !~ m!encoded (\d+) frames, (\d+\.\d+) fps!)
    {
        print "error: unparseable log summary info \n"; return +{};
    }
    $stats->{num_frames} = $1;
    $stats->{fps} = $2;

    return $stats;
}

sub read_file
{
    my ($file) = @_;
    open(F, $file) || die "could not open $file: $!";
    undef $/;
    my $data = <F>;
    $/ = "\n";
    close(F);
    return $data;
}

sub write_file
{
    my ($file, $data) = @_;
    open(F, ">".$file) || die "could not open $file: $!";
    print F $data;
    close(F);
}
