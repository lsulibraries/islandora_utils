<?php

function format_times($timer,$elapsed){
    if(empty($timer)) return;
    return sprintf("   %f   |   Avg: %f   |    Elapsed: %f", $elapsed/1000, $timer['time']/$timer['count']/1000, $timer['time']/1000);
}

/**
 * Query the RI index to get PIDs of all child objects in a collection.
 *
 * @param string $pid
 *   The PID of Islandora collection.
 *
 * @return array
 *   List of member object PIDs.
 */
function get_collection_members($pid) {

  $members = array();
  // Query the rindex to get all the objects that have a 'isMemberOfCollection'
  // relationship with the specified collection and add all their PIDs to the
  // list.
  $tuque = islandora_get_tuque_connection();
  $islandora_rels_ext = ISLANDORA_RELS_EXT_URI;
  $ri_query = <<<EOF
select ?object from <#ri> where {
  {
    ?object <fedora-rels-ext:isMemberOfCollection> <info:fedora/$pid> .
  } UNION {
    ?object <{$islandora_rels_ext}isPageOf> ?book .
    ?book <fedora-rels-ext:isMemberOfCollection> <info:fedora/$pid> .
  } UNION {
    ?object <fedora-rels-ext:isConstituentOf> ?compound .
    ?compound <fedora-rels-ext:isMemberOfCollection> <info:fedora/$pid> .
  } UNION {
    ?object <fedora-rels-ext:isMemberOf> ?issue .
    ?issue <fedora-rels-ext:isMemberOf> ?newspaper .
    ?newspaper <fedora-rels-ext:isMemberOfCollection> <info:fedora/$pid> .
  }

}
EOF;
  $result = $tuque->repository->ri->sparqlQuery($ri_query, 'unlimited');
  foreach ($result as $member) {
    $member_pid = preg_replace('/info:fedora\//', '', $member['object']['value']);
    $members[] = $member_pid;
  }
  return $members;
}


/**
 * Sends a request to GSearch.
 *
 * @param string $pid
 *   The PID to send.
 * @param string $opp
 *   The operation to do.
 */
function speak_to_gsearch($pid, $opp) {
  if ($opp == 'index') {
    $action = 'fromPid';
    $verb = 'index';
  }
  elseif ($opp == 'purge') {
    $action = 'deletePid';
    $verb = 'purge';
  }

  $password = variable_get('islandora_gsearcher_gsearch_pass', 'fedoraAdmin');
  $user = variable_get('islandora_gsearcher_gsearch_user', 'fedoraAdmin');
  $url = variable_get('islandora_gsearcher_gsearch_url', 'localhost:8080/fedoragsearch/rest');

  $response = drupal_http_request(
    url(
      "http://$user:$password@$url",
      array(
        'query' => array(
          'operation' => 'updateIndex',
          'action' => $action,
          'value' => $pid,
        ),
      )
    ),array('timeout' => 5)
  );
  // Ping the logs in case of failure, GSearch will know more.
  if ($response->code != 200) {
    watchdog(
      'Islandora GSearcher',
      'GSearch did not return 200 when asked to @verb @pid.',
      array('@verb' => $verb, '@pid' => $pid),
      WATCHDOG_ERROR
    );
    return false;
  }
  return true;
}
