
?
Command: %s
53*	vivadotcl2
route_designZ4-113h px� 
�
@Attempting to get a license for feature '%s' and/or device '%s'
308*common2
Implementation2

xc7a200tZ17-347h px� 
p
0Got license for feature '%s' and/or device '%s'
310*common2
Implementation2

xc7a200tZ17-349h px� 
D

Starting %s Task
103*constraints2	
RoutingZ18-103h px� 
k
BMultithreading enabled for route_design using a maximum of %s CPUs17*	routeflow2
8Z35-254h px� 
L

Phase %s%s
101*constraints2
1 2
Build RT DesignZ18-101h px� 
I
%s*common20
.Phase 1 Build RT Design | Checksum: 24fbfd0e8
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:13 ; elapsed = 00:01:01 . Memory (MB): peak = 2935.098 ; gain = 30.934 ; free physical = 5118 ; free virtual = 14019h px� 
R

Phase %s%s
101*constraints2
2 2
Router InitializationZ18-101h px� 
W

Phase %s%s
101*constraints2
2.1 2
Fix Topology ConstraintsZ18-101h px� 
T
%s*common2;
9Phase 2.1 Fix Topology Constraints | Checksum: 24fbfd0e8
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:13 ; elapsed = 00:01:01 . Memory (MB): peak = 2935.098 ; gain = 30.934 ; free physical = 5118 ; free virtual = 14019h px� 
P

Phase %s%s
101*constraints2
2.2 2
Pre Route CleanupZ18-101h px� 
M
%s*common24
2Phase 2.2 Pre Route Cleanup | Checksum: 24fbfd0e8
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:13 ; elapsed = 00:01:01 . Memory (MB): peak = 2935.098 ; gain = 30.934 ; free physical = 5118 ; free virtual = 14019h px� 
L

Phase %s%s
101*constraints2
2.3 2
Update TimingZ18-101h px� 
I
%s*common20
.Phase 2.3 Update Timing | Checksum: 2a3137b32
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:18 ; elapsed = 00:01:02 . Memory (MB): peak = 3002.855 ; gain = 98.691 ; free physical = 5048 ; free virtual = 13949h px� 
{
Intermediate Timing Summary %s164*route2:
8| WNS=-3.618 | TNS=-2119.828| WHS=-0.233 | THS=-92.739|
Z35-416h px� 
O
%s*common26
4Phase 2 Router Initialization | Checksum: 2810ac1c8
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:21 ; elapsed = 00:01:03 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13941h px� 
K

Phase %s%s
101*constraints2
3 2
Global RoutingZ18-101h px� 
H
%s*common2/
-Phase 3 Global Routing | Checksum: 2810ac1c8
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:21 ; elapsed = 00:01:03 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13941h px� 
L

Phase %s%s
101*constraints2
4 2
Initial RoutingZ18-101h px� 
W

Phase %s%s
101*constraints2
4.1 2
Initial Net Routing PassZ18-101h px� 
T
%s*common2;
9Phase 4.1 Initial Net Routing Pass | Checksum: 28ae5c4e7
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:30 ; elapsed = 00:01:06 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13941h px� 
I
%s*common20
.Phase 4 Initial Routing | Checksum: 28ae5c4e7
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:30 ; elapsed = 00:01:06 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13941h px� 
�
>Design has %s pins with tight setup and hold constraints.

%s
244*route2
1932�
�The top 5 pins with tight setup and hold constraints:

+====================+===================+===================================+
| Launch Setup Clock | Launch Hold Clock | Pin                               |
+====================+===================+===================================+
| sys_pll_out_1      | sys_pll_out_1     | u_cpu/u_cpu/mem_wordsize_reg[1]/D |
| sys_pll_out_1      | sys_pll_out_1     | u_cpu/u_cpu/mem_wordsize_reg[0]/D |
| sys_pll_out_1      | sys_pll_out_1     | u_cpu/u_cpu/cpu_state_reg[1]/D    |
| sys_pll_out_1      | sys_pll_out       | u_cpu/u_cpu/is_alu_reg_reg_reg/D  |
| sys_pll_out_1      | sys_pll_out_1     | u_cpu/u_cpu/is_sb_sh_sw_reg/D     |
+--------------------+-------------------+-----------------------------------+

File with complete list of pins: tight_setup_hold_pins.txt
Z35-580h px� 
O

Phase %s%s
101*constraints2
5 2
Rip-up And RerouteZ18-101h px� 
Q

Phase %s%s
101*constraints2
5.1 2
Global Iteration 0Z18-101h px� 
{
Intermediate Timing Summary %s164*route2:
8| WNS=-4.407 | TNS=-3434.328| WHS=N/A    | THS=N/A    |
Z35-416h px� 
N
%s*common25
3Phase 5.1 Global Iteration 0 | Checksum: 1d32f557d
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:40 ; elapsed = 00:01:12 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
Q

Phase %s%s
101*constraints2
5.2 2
Global Iteration 1Z18-101h px� 
{
Intermediate Timing Summary %s164*route2:
8| WNS=-4.477 | TNS=-3362.578| WHS=N/A    | THS=N/A    |
Z35-416h px� 
N
%s*common25
3Phase 5.2 Global Iteration 1 | Checksum: 2992734fa
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:47 ; elapsed = 00:01:16 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
L
%s*common23
1Phase 5 Rip-up And Reroute | Checksum: 2992734fa
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:47 ; elapsed = 00:01:16 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
X

Phase %s%s
101*constraints2
6 2
Delay and Skew OptimizationZ18-101h px� 
L

Phase %s%s
101*constraints2
6.1 2
Delay CleanUpZ18-101h px� 
N

Phase %s%s
101*constraints2
6.1.1 2
Update TimingZ18-101h px� 
K
%s*common22
0Phase 6.1.1 Update Timing | Checksum: 1ff7a2e2c
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:47 ; elapsed = 00:01:16 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
{
Intermediate Timing Summary %s164*route2:
8| WNS=-4.333 | TNS=-3288.427| WHS=N/A    | THS=N/A    |
Z35-416h px� 
I
%s*common20
.Phase 6.1 Delay CleanUp | Checksum: 20752fe9f
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:49 ; elapsed = 00:01:17 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
V

Phase %s%s
101*constraints2
6.2 2
Clock Skew OptimizationZ18-101h px� 
S
%s*common2:
8Phase 6.2 Clock Skew Optimization | Checksum: 20752fe9f
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:49 ; elapsed = 00:01:17 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
U
%s*common2<
:Phase 6 Delay and Skew Optimization | Checksum: 20752fe9f
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:49 ; elapsed = 00:01:17 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
J

Phase %s%s
101*constraints2
7 2
Post Hold FixZ18-101h px� 
L

Phase %s%s
101*constraints2
7.1 2
Hold Fix IterZ18-101h px� 
{
Intermediate Timing Summary %s164*route2:
8| WNS=-4.316 | TNS=-3129.897| WHS=0.055  | THS=0.000  |
Z35-416h px� 
I
%s*common20
.Phase 7.1 Hold Fix Iter | Checksum: 23bacace3
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:51 ; elapsed = 00:01:17 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
G
%s*common2.
,Phase 7 Post Hold Fix | Checksum: 23bacace3
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:51 ; elapsed = 00:01:17 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
K

Phase %s%s
101*constraints2
8 2
Route finalizeZ18-101h px� 
H
%s*common2/
-Phase 8 Route finalize | Checksum: 23bacace3
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:51 ; elapsed = 00:01:17 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
R

Phase %s%s
101*constraints2
9 2
Verifying routed netsZ18-101h px� 
O
%s*common26
4Phase 9 Verifying routed nets | Checksum: 23bacace3
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:51 ; elapsed = 00:01:17 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
O

Phase %s%s
101*constraints2
10 2
Depositing RoutesZ18-101h px� 
L
%s*common23
1Phase 10 Depositing Routes | Checksum: 1b6616873
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:52 ; elapsed = 00:01:18 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
R

Phase %s%s
101*constraints2
11 2
Post Process RoutingZ18-101h px� 
O
%s*common26
4Phase 11 Post Process Routing | Checksum: 1b6616873
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:52 ; elapsed = 00:01:18 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
P

Phase %s%s
101*constraints2
12 2
Post Router TimingZ18-101h px� 
w
Estimated Timing Summary %s
57*route2:
8| WNS=-4.316 | TNS=-3129.897| WHS=0.055  | THS=0.000  |
Z35-57h px� 
B
!Router estimated timing not met.
128*routeZ35-328h px� 
M
%s*common24
2Phase 12 Post Router Timing | Checksum: 1b6616873
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:52 ; elapsed = 00:01:18 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
Y

Phase %s%s
101*constraints2
13 2
Post-Route Event ProcessingZ18-101h px� 
V
%s*common2=
;Phase 13 Post-Route Event Processing | Checksum: 1ab3aea26
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:52 ; elapsed = 00:01:18 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
@
Router Completed Successfully
2*	routeflowZ35-16h px� 
E
%s*common2,
*Ending Routing Task | Checksum: 1ab3aea26
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:01:52 ; elapsed = 00:01:18 . Memory (MB): peak = 3010.246 ; gain = 106.082 ; free physical = 5040 ; free virtual = 13935h px� 
H
Releasing license: %s
83*common2
ImplementationZ17-83h px� 

G%s Infos, %s Warnings, %s Critical Warnings and %s Errors encountered.
28*	vivadotcl2
1012
12
02
0Z4-41h px� 
L
%s completed successfully
29*	vivadotcl2
route_designZ4-42h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
route_design: 2

00:01:522

00:01:182

3010.2462	
106.0822
50402
13935Z17-722h px� 
�
Executing command : %s
56330*	planAhead2S
Qreport_drc -file top_drc_routed.rpt -pb top_drc_routed.pb -rpx top_drc_routed.rpxZ12-24828h px� 
�
Command: %s
53*	vivadotcl2S
Qreport_drc -file top_drc_routed.rpt -pb top_drc_routed.pb -rpx top_drc_routed.rpxZ4-113h px� 
>
IP Catalog is up to date.1232*coregenZ19-1839h px� 
>
Running DRC with %s threads
24*drc2
8Z23-27h px� 
�
#The results of DRC are in file %s.
586*	vivadotcl2�
n/home/minela/Projects/Work/uberClock/3.miniac/3.build/hw_build.Vivado/uberclock.runs/impl_1/top_drc_routed.rptn/home/minela/Projects/Work/uberClock/3.miniac/3.build/hw_build.Vivado/uberclock.runs/impl_1/top_drc_routed.rpt8Z2-168h px� 
J
%s completed successfully
29*	vivadotcl2

report_drcZ4-42h px� 
�
Executing command : %s
56330*	planAhead2
}report_methodology -file top_methodology_drc_routed.rpt -pb top_methodology_drc_routed.pb -rpx top_methodology_drc_routed.rpxZ12-24828h px� 
�
Command: %s
53*	vivadotcl2
}report_methodology -file top_methodology_drc_routed.rpt -pb top_methodology_drc_routed.pb -rpx top_methodology_drc_routed.rpxZ4-113h px� 
E
%Done setting XDC timing constraints.
35*timingZ38-35h px� 
G
$Running Methodology with %s threads
74*drc2
8Z23-133h px� 
�
2The results of Report Methodology are in file %s.
609*	vivadotcl2�
z/home/minela/Projects/Work/uberClock/3.miniac/3.build/hw_build.Vivado/uberclock.runs/impl_1/top_methodology_drc_routed.rptz/home/minela/Projects/Work/uberClock/3.miniac/3.build/hw_build.Vivado/uberclock.runs/impl_1/top_methodology_drc_routed.rpt8Z2-1520h px� 
R
%s completed successfully
29*	vivadotcl2
report_methodologyZ4-42h px� 
�
Executing command : %s
56330*	planAhead2�
�report_timing_summary -max_paths 10 -file top_timing_summary_routed.rpt -pb top_timing_summary_routed.pb -rpx top_timing_summary_routed.rpx -warn_on_violation Z12-24828h px� 
E
%Done setting XDC timing constraints.
35*timingZ38-35h px� 
`
UpdateTimingParams:%s.
91*timing2'
% Speed grade: -2, Delay Type: min_maxZ38-91h px� 
j
CMultithreading enabled for timing update using a maximum of %s CPUs155*timing2
8Z38-191h px� 
�
rThe design failed to meet the timing requirements. Please see the %s report for details on the timing violations.
188*timing2
timing summaryZ38-282h px� 
�
)Running report commands "%s" in parallel.56334*	planAhead2@
>report_bus_skew, report_incremental_reuse, report_route_statusZ12-24838h px� 
Y
+Running report generation with %s threads.
56333*	planAhead2
3Z12-24831h px� 
�
Executing command : %s
56330*	planAhead2H
Freport_route_status -file top_route_status.rpt -pb top_route_status.pbZ12-24828h px� 
�
Executing command : %s
56330*	planAhead2A
?report_incremental_reuse -file top_incremental_reuse_routed.rptZ12-24828h px� 
g
BIncremental flow is disabled. No incremental reuse Info to report.423*	vivadotclZ4-1062h px� 
�
Executing command : %s
56330*	planAhead2z
xreport_bus_skew -warn_on_violation -file top_bus_skew_routed.rpt -pb top_bus_skew_routed.pb -rpx top_bus_skew_routed.rpxZ12-24828h px� 
`
UpdateTimingParams:%s.
91*timing2'
% Speed grade: -2, Delay Type: min_maxZ38-91h px� 
j
CMultithreading enabled for timing update using a maximum of %s CPUs155*timing2
8Z38-191h px� 
�
Executing command : %s
56330*	planAhead2c
areport_power -file top_power_routed.rpt -pb top_power_summary_routed.pb -rpx top_power_routed.rpxZ12-24828h px� 
�
Command: %s
53*	vivadotcl2c
areport_power -file top_power_routed.rpt -pb top_power_summary_routed.pb -rpx top_power_routed.rpxZ4-113h px� 
K
,Running Vector-less Activity Propagation...
51*powerZ33-51h px� 
P
3
Finished Running Vector-less Activity Propagation
1*powerZ33-1h px� 

G%s Infos, %s Warnings, %s Critical Warnings and %s Errors encountered.
28*	vivadotcl2
1212
12
12
0Z4-41h px� 
L
%s completed successfully
29*	vivadotcl2
report_powerZ4-42h px� 
�
Executing command : %s
56330*	planAhead2A
?report_clock_utilization -file top_clock_utilization_routed.rptZ12-24828h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
generate_parallel_reports: 2

00:00:442

00:00:102

3106.4882
96.2422
50002
13899Z17-722h px� 
H
&Writing timing data to binary archive.266*timingZ38-480h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Write ShapeDB Complete: 2
00:00:00.032
00:00:00.012

3106.4882
0.0002
50002
13900Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote PlaceDB: 2
00:00:00.542
00:00:00.172

3106.4882
0.0002
49962
13898Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote PulsedLatchDB: 2

00:00:002

00:00:002

3106.4882
0.0002
49962
13898Z17-722h px� 
=
Writing XDEF routing.
211*designutilsZ20-211h px� 
J
#Writing XDEF routing logical nets.
209*designutilsZ20-209h px� 
J
#Writing XDEF routing special nets.
210*designutilsZ20-210h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote RouteStorage: 2
00:00:00.092
00:00:00.052

3106.4882
0.0002
49932
13895Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote Netlist Cache: 2

00:00:002
00:00:00.012

3106.4882
0.0002
49932
13896Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote Device Cache: 2
00:00:00.012

00:00:002

3106.4882
0.0002
49932
13896Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Write Physdb Complete: 2
00:00:00.642
00:00:00.242

3106.4882
0.0002
49932
13896Z17-722h px� 
�
 The %s '%s' has been generated.
621*common2

checkpoint2l
j/home/minela/Projects/Work/uberClock/3.miniac/3.build/hw_build.Vivado/uberclock.runs/impl_1/top_routed.dcpZ17-1381h px� 


End Record