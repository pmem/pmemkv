// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/* This file contains code for pipelines using restore snaphosts. */

/**
 * Function will run timeshift app on a remote server, using SSH agent plugin.
 * @param ssh_credentials Specified ssh credentials. Must be set in Jenkins first.
 * @param user_name User that ssh will try to log on.
 * @param ssh_address IP address of remote machine.
 * @param snapshot_name Specified name of a Timeshift snapshot.
 */
def restore_snapshot(ssh_credentials, user_name, ssh_address, snapshot_name) {
/*
	Expecting to lose connection, when server is rebooted.
	printf simulates user input necessary to use Timeshift.
*/
	sshagent (credentials: [ssh_credentials]) {
		libs.api.run_bash_script("""
			ssh -o ServerAliveInterval=2 -o ServerAliveCountMax=2 -l ${user_name} ${ssh_address}\\
			 "printf '\ny\n0\ny\n' | sudo timeshift --restore --snapshot ${snapshot_name}" || true
		""")
	}
}

/**
 * Function will test and wait for connection to remote server.
 * @param ssh_credentials Specified ssh credentials. Must be set in Jenkins first.
 * @param user_name User that ssh will try to log on.
 * @param ssh_address IP address of remote machine.
 * @param no_of_tries Number of reconnect tries.
 * @param con_timeout value of ssh ConnectTimeout parameter.
 * @param con_attempts value of ssh ConnectionAttempts parameter.
 */
def wait_for_reconnect(ssh_credentials, user_name, ssh_address, no_of_tries = 200, con_timeout = 1, con_attempts = 1) {
	sshagent (credentials: [ssh_credentials]) {
		libs.api.run_bash_script("""
			function wait_for_connection() {
				ssh -o ConnectTimeout=${con_timeout} -o ConnectionAttempts=${con_attempts}\\
				 -o StrictHostKeyChecking=no -l ${user_name} ${ssh_address} 'true'
				echo \$?
			}
			for (( i=${no_of_tries}; i>0; i-- ))
			do
				echo "trying to reach ${ssh_address}. \$i attempts left"
				if [ "\$(wait_for_connection)" -eq 0 ]; then
					break
				elif [ "\$i" -eq "1" ]; then
					echo "ERROR: Could not reach ${ssh_address}"
					exit 1
				fi
				sleep 1
			done
			sleep 10
		""")
	}
}

/**
 * Reconnect and bring online specified node defined in Jenkins.
 * @param node_name node name as defined in Jenkins.
 */
def reconnect_node(node_name) {
	for (jenkins_node in jenkins.model.Jenkins.instance.slaves) {
		 if (jenkins_node.name == node_name) {
			println('Bringing node online.')
			jenkins_node.getComputer().connect(true)
			jenkins_node.getComputer().setTemporarilyOffline(false)
			break;
		}
	}
}

/**
 * Disconnect and set offline specified node defined in Jenkins.
 * @param node_name node name as defined in Jenkins.
 */
def diconnect_node(node_name) {
	for (jenkins_node in jenkins.model.Jenkins.instance.slaves) {
		 if (jenkins_node.name == node_name) {
			println('Disconnecting node')
			jenkins_node.getComputer().disconnect(new hudson.slaves.OfflineCause.ByCLI("Disconnected by Jenkins"))
		}
	}
}

/**
 * Returns Jenkins node IP address.
 * @param node_name node name as defined in Jenkins.
 * @return string with Jenkins node IP address.
 */
def get_node_address(node_name) {
	for (jenkins_node in jenkins.model.Jenkins.instance.slaves) {
		if (jenkins_node.name == node_name) {
			return jenkins_node.getLauncher().getHost()
		}
	}
}

/**
 * Returns Timeshift snapshot name from the server.
 * @param snapshot_option Option on which to chose proper Timeshift snapshot.
 	SKIP: skip selecting snapshot name
 	JENKINS_LATEST: A snapshot with description 'Jenkins_backup_latest'
 	will be selected.
 	LATEST: Latest snapshot on the target server will be selected.
 	CHOOSE: A specified snapshot name is chosen, passed in the 'snapshot_option_name'.
 * @param snapshot_option_name If 'CHOOSE' option is selected then the string
 	in the snapshot_option_name will be chosen as snapshot name.
 * @return String Snapshot name.
 */
def get_snapshot_name(snapshot_option, snapshot_option_name='' ) {
	def snapshot_name = ''
	switch(snapshot_option) {
		case 'SKIP':
			break
		case 'JENKINS_LATEST':
			snapshot_name = libs.api.run_bash_script("""
				res=\$(sudo timeshift --list-snapshots | grep Jenkins_backup_latest)
				echo \$res | awk '{print \$3}' | head -n 1
			""").output
			break
		case 'LATEST':
			snapshot_name = libs.api.run_bash_script("""
				res=\$(sudo timeshift --list-snapshots | tail -n 2)
				echo \$res | awk '{print \$3}'
			""").output
		break
		case 'CHOOSE':
			snapshot_name = snapshot_option_name
			break
		default:
			throw new java.lang.UnsupportedOperationException("'${snapshot_option}' is not a valid option.")
	}
	return snapshot_name
}
return this
