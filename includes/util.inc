<?php
require_once('gsearch.inc');

function index_pids($list){

    $item_count = count($list);
    $last = 0;
    $failures = array();

    foreach($list as $pid){
        timer_start('gsearch');

        if(!speak_to_gsearch($pid, 'index')){
            $failures[] = $pid;
        }

        $timer   = timer_stop('gsearch');
        $elapsed = $timer['time'] - $last;
        drush_log("Reindexed $pid.       ". format_times($timer, $elapsed), 'success');

        $last    = $timer['time'];
    }
    if(!empty($failures)){
        $cnt = count($failures);
        drush_log("$cnt items failed or timed-out during indexing\n". output_result($failures), 'warning');
    }else{
        drush_log("All ($item_count) items were reindexed successfully.", "success");
    }
}

function csv_format_pid_list($pids){
    if(!is_array($pids)){
        throw new Exception('Input param is not an array.');
    }
    return implode(',',$pids);
}

/**
 * 
 * @param array $pids
 */
function output_result($pids){
    $file = drush_get_option('outfile');
    $pidstr = csv_format_pid_list($pids);
    if($file){
        print("See file $file\n");
        file_put_contents($file, $pidstr);
    }else{
        print("\n".$pidstr."\n\n");
    }
}