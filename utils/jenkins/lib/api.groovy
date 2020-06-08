//
// Copyright 2019-2020, Intel Corporation
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in
//       the documentation and/or other materials provided with the
//       distribution.
//
//     * Neither the name of the copyright holder nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

// This file contains common API for all our Jenkins pipelines.

import java.util.regex.*

// declarations of common paths and filenames:
WORKSPACE_DIR = '${WORKSPACE}' // do not change '${WORKSPACE}' to "${WORKSPACE}" - string variable should be made of ${WORKSPACE} string to allow expansion on the DUT's OS shell
OUTPUT_DIR_NAME = 'output'
RESULTS_DIR_NAME = 'results'

OUTPUT_DIR = "${WORKSPACE_DIR}/${OUTPUT_DIR_NAME}"
RESULTS_DIR = "${WORKSPACE_DIR}/${RESULTS_DIR_NAME}"
SCRIPTS_DIR = "${WORKSPACE_DIR}/jenkins_files/utils/jenkins/scripts"

LOG_FILENAME = 'console.log'
LOG_FILE = "${OUTPUT_DIR}/${LOG_FILENAME}"

SYSTEM_INFO_FILENAME = 'system_info.txt'
SYSTEM_INFO_FILEPATH = "${RESULTS_DIR}/${SYSTEM_INFO_FILENAME}"

COMMON_FILE = "${SCRIPTS_DIR}/common.sh"

//interpolation function for log absolute filepath
def abs_logfile_path() {
	return "${WORKSPACE}/${OUTPUT_DIR_NAME}/${LOG_FILENAME}"
}
// low-level functions for basic interaction with OS on the DUT:

// Required to use top-level function inside class method
TestsResult.metaClass.get_work_week_of_build_start = { get_work_week_of_build_start() }

/**
 * Function which runs a script in bash and logs it's output.
 *
 * @param script_text Bash script's contents. No need for shebang, pass only what to do.
 * @param log Path to the file where script's output will be redirected. If empty string, then no output redirection will be used. Default set to LOG_FILE.
 * @param import_common_file Boolean flag, if set to true the 'common.sh' script will be included.
 * @param error_on_non_zero_rc Boolean flag, if set to true function will raise error when command returns non-zero return code.
 * @return map object containing '.output' with command's output and '.status' with returned command's status code
 */
def run_bash_script(script_text, log_file = LOG_FILE, import_common_file = false, error_on_non_zero_rc = true) {
	// first, prepare optional portions of the script:
	// redirect all script's output to the files:
	// 1. Separate log file for this current command exclusively, with random name.
	// 2. If requested - append to given log file.
	def current_script_output_file = "command_" + generate_random_string(20) + ".log"
	def redirect_script = """
		# ensure that this command output file is empty:
		echo "" > ${current_script_output_file}
		# ensure that log_file exists before using `tee -a`:
		touch ${log_file} ${current_script_output_file}
		# redirect stdout to the named pipe:
		exec > >(tee -a ${log_file} ${current_script_output_file})
		# merge stderr stream with stdout:
		exec 2>&1
	"""

	// import things defined in common.sh file if not set otherwise:
	def source_script = (import_common_file == false) ? "" : "source ${COMMON_FILE}"

	def full_script_body =  """#!/usr/bin/env bash
		set -o pipefail
		${redirect_script}
		${source_script}
		# now do whatever user wanted to:
		${script_text}
	"""
	// second, do the actual call:
	def returned_status = sh(script: full_script_body, returnStatus: true)

	// third, capture script output from file:
	def script_output = readFile current_script_output_file

	// delete the log file:
	sh "rm -f ${current_script_output_file}"

	// capturing status is also disabling default behavior of `sh` - error when exit status != 0
	// here restore that behavior if not requested otherwise:
	if (error_on_non_zero_rc && returned_status != 0) {
		error("script returned exit code ${returned_status}")
	}
	
	def retval = [:]
	retval.output = script_output.trim()
	retval.status = returned_status
	return retval
}

/**
 *  Function which runs a script in bash and logs it's output. There will be always 'common.sh' script included.
 * @param script_text Bash script's contents. No need for shebang, pass only what to do.
 * @param log Path to the file where script's output will be redirected. If empty string, then no output redirection will be used. Default set to LOG_FILE.
 * @param error_on_non_zero_rc Boolean flag, if set to true function will raise error when command returns non-zero return code.
 * @return object containing '.output' with command's output and '.status' with returned command's status code
 */
def run_bash_script_with_common_import(script_text, log = LOG_FILE, error_on_non_zero_rc = true) {
	return run_bash_script(script_text, log, true, error_on_non_zero_rc)
}

/**
 * Function which runs CMake.
 * @param path A path pointing to directory with CMakeLists.txt file.
 * @param parameters Additional parameters for CMake configuration.
 * @param additional_env_vars If some additional exports are needed before call CMake, put there here.
 */
def run_cmake(path, parameters = "", additional_env_vars = "") {
	if (additional_env_vars != "") {
		additional_env_vars = "export ${additional_env_vars} &&"
	}
	run_bash_script("${additional_env_vars} cmake ${path} ${parameters}")
}

/**
 * Function which runs make on as many threads as available cores.
 * @param parameters Additional parameters for CMake configuration.
 * @param additional_env_vars If some additional exports are needed before call make, put there here.
 */
def run_make(parameters = "", additional_env_vars = "") {
	if (additional_env_vars != "") {
		additional_env_vars = "export ${additional_env_vars} &&"
	}
	run_bash_script("${additional_env_vars} make -j\$(nproc) ${parameters}")
}

/**
 * Do a bash call with 'echo' and provided text.
 * @param text String to print in bash.
 */
def bash_echo(text) {
	run_bash_script("echo \"${text}\"")
}

/**
 * Prints given text as a visible header in Jenkins log and bash.
 * @param text String to print in header.
 */
def echo_header(text) {
	def header = "*********************************** ${text} ***********************************"
	// echo to Jenkins log:
	echo header

	// echo to console output:
	bash_echo(header)
}

/**
 * Generate random alphanumeric string.
 * @param length Number of characters to generate.
 */
def generate_random_string(length) {
	String chars = (('a'..'z') + ('A'..'Z') + ('0'..'9')).join('')
	Random rnd = new Random()
	def random_string = ""
	for(i = 0; i < length; i++) {
		random_string = random_string + chars.charAt(rnd.nextInt(chars.length()))
	}
	return random_string
}

// higher-level functions for dealing with parts of pipelines' stages:

/**
 * Download repository with git.
 * @param repo_url URL to requested repository.
 * @param branch Specified branch to checkout.
 * @param target_directory Directory to which will the repository be clonned. If not specified, repository will be cloned to current working directory.
 */
def clone_repository(repo_url, branch, target_directory = '') {
	def specified_target_dir = (target_directory == '') ? false : true

	// If target dir is not specified, then we will create temporary dir and then copy contents to current dir.
	// The reason is: checkout will remove all files from current dir if its not empty.
	if(!specified_target_dir) {
		target_directory = "repository_target_temp_${generate_random_string(8)}"
	}

	checkout([$class: 'GitSCM', branches: [[name: "${branch}"]], doGenerateSubmoduleConfigurations: false, extensions: [[$class: 'RelativeTargetDirectory', relativeTargetDir: target_directory]], submoduleCfg: [], userRemoteConfigs: [[url: repo_url]]])

	echo_header("Git log")

	run_bash_script("cd ${target_directory} && git log --oneline -n 5")

	if(!specified_target_dir) {
		run_bash_script("""
			# move all contents of target_directory (including hidden files) to the current dir:
			mv -f ${target_directory}/{.[!.],}* ./

			# delete target directory:
			rmdir ${target_directory}
		""")
	}
}

/**
 * Clone the pmdk-tests repository to the 'pmdk-tests' directory.
 * @param branch The branch (or tag or revision) for checking out to.
 */
def clone_pmdk_test(branch = "master") {
	clone_repository('https://github.com/pmem/pmdk-tests.git', branch, 'pmdk-tests')
}

/**
 * Set a moose as a welcome message after logging to the DUT, which will warn about running execution.
 */
def set_jenkins_warning_on_dut() {
	run_bash_script_with_common_import("set_warning_message")
}

/**
 * Restore default welcome message after logging to the DUT which means that Jenkins' execution is done.
 */
def unset_jenkins_warning_on_dut() {
	run_bash_script_with_common_import("disable_warning_message")
}

/**
 * Print OS and BRANCH params of current job to Jenkins. Call script which collects system info and save it to logs.
 */
def system_info() {
	echo_header("system info")

	dir(RESULTS_DIR_NAME) {
		writeFile file: SYSTEM_INFO_FILENAME, text: ''
	}

	print "OS: ${params.LABEL}"
	print "Branch: ${params.BRANCH}"

	run_bash_script_with_common_import("""
		system_info 2>&1 | tee -a ${SYSTEM_INFO_FILEPATH}
	""")
}

/**
 * Enumeration class for distinction between test types.
 */
enum TestType {
	/**
	 * Value describing unittests set from PMDK repository.
	 */
	UNITTESTS('testconfig.sh', 'u'),

	/**
	 * Value describing tests for building, installing and using binaries from PKG (PMDK repository).
	 */
	PKG('testconfig.sh', 'r'),

	/**
	 * Value describing unittests set in python from PMDK repository.
	 */
	UNITTESTS_PY('testconfig.py', 'p'),

	/**
	 * Value describing test set from PMDK-test repository.
	 */
	PMDK_TESTS('config.xml', 't'),

	/**
	 * Value describing test set from PMDK-convert repository.
	 */
	PMDK_CONVERT('convertConfig.txt', 'c')

	/**
	 * Constructor for assigning proper values to enumerators' properties.
	 * @param path_to_config Path to the config generated by createNamescpacesConfig.sh script.
	 * @param script_parameter Parameter which is passed to the createNamespacesConfig.sh script.
	 */
	TestType(String path_to_config, String script_parameter) {
		this.config_path = path_to_config
		this.script_parameter = script_parameter
	}

	/**
	 * Getter for property describing path to the config generated by createNamescpacesConfig.sh script.
	 */
	def get_config_path() {
		return this.config_path
	}

	/**
	 * Getter for property describing parameter which is passed to the createNamespacesConfig.sh script.
	 */
	def get_script_parameter() {
		return this.script_parameter
	}

	/**
	 * Property holding path to the config generated by createNamescpacesConfig.sh script.
	 */
	private final String config_path

	/**
	 * Property holding parameter which is passed to the createNamespacesConfig.sh script.
	 */
	private final String script_parameter
}

// "export" this enum type;
// below is necessary in order to closure work properly with enum after loading the file in the pipeline:
this.TestType = TestType

/**
 * Enumeration for linux distributions.
 */
enum DistroName {

	UNKNOWN(''),
	DEBIAN('Debian GNU/Linux'),
	CENTOS('CentOS Linux'),
	OPENSUSE('openSUSE Leap'),
	FEDORA('Fedora'),
	UBUNTU('Ubuntu'),
	RHEL('Red Hat Enterprise Linux'),
	RHELS('Red Hat Enterprise Linux Server')

	/**
	 * Constructor for assigning proper values to enumerators' properties.
	 * @param name Name of the linux distribution.
	 */
	DistroName(String distro_name) {
		this.distro_name = distro_name
	}

	def to_string() {
		return this.distro_name
	}

	static def from_string(string_distro_name) {
		// lookup in all enum labels and check if given string matches:
		for (DistroName distro_name : DistroName.values()) {
			if (distro_name.to_string() == string_distro_name) {
				return distro_name
			}
		}
		return DistroName.UNKNOWN
	}

	private final String distro_name
}

// "export" this enum type;
// below is necessary in order to closure work properly with enum after loading the file in the pipeline:
this.DistroName = DistroName

/**
 * Call createNamespaceConfig.sh and print created configuration files.
 * @param conf_path Path to directory where config files will be created.
 * @param nondebug_lib_path path to installed pmdk libraries. Required if the tests have to be run with precompiled binaries.
 */
def create_namespace_and_config(conf_path, TestType test_type, second_conf_path = '', nondebug_lib_path = '') {
	echo_header("Setup & create config")

	def dual_config_string = (second_conf_path == '') ? '' : "--dual-ns --conf-path_1=${second_conf_path}"

	run_bash_script_with_common_import("""
		mkdir --parents ${conf_path}
		${SCRIPTS_DIR}/createNamespaceConfig.sh -${test_type.get_script_parameter()} --conf-pmdk-nondebug-lib-path=${nondebug_lib_path} --conf-path_0=${conf_path} ${dual_config_string}
	""")

	echo_header(test_type.get_config_path())
	run_bash_script("cat ${conf_path}/${test_type.get_config_path()}")
	if ( second_conf_path != '' ) {
		run_bash_script("cat ${second_conf_path}/${test_type.get_config_path()}")
	}
}

/**
 * Archive artifacts from output directory.
 */
def archive_output() {
	archiveArtifacts artifacts: "${OUTPUT_DIR_NAME}/*.*"
}

/**
 * Archive artifacts from output and results directory.
 */
def archive_results_and_output() {
	archive_output()
	archiveArtifacts artifacts: "${RESULTS_DIR_NAME}/*.*"
}

/**
 * Write 'result' file (success, fail) and archive it.
 * @param result String representing result (could be e.g. 'success' or 'fail').
 */
def write_result_and_archive(result) {
	dir(RESULTS_DIR_NAME) {
		writeFile file: "${result}.txt", text: result
	}
	archiveArtifacts artifacts: "${RESULTS_DIR_NAME}/*.*"
}

/**
 * Write file with name OS_BRANCH.txt to results dir.
 */
def write_os_and_branch(os, branch = null) {
	def branch_chunk = (branch == null) ? "" : "_${branch}"
	def filename = "${os}${branch_chunk}.txt"
	dir(RESULTS_DIR_NAME) {
		writeFile file: filename, text: ''
	}
}

/**
 * Clone repository with PMDK sources to the current directory.
 * @param branch Branch to clone.
 */
def clone_pmdk_repo(branch) {
	clone_repository('https://github.com/pmem/pmdk.git', branch)
}

/**
 * Run PaJaC with proper parameters.
 */
def run_pajac(input_filepath, output_filepath) {
	run_bash_script("${SCRIPTS_DIR}/pajac/converter.py ${input_filepath} ${output_filepath}")
}

/**
 * Get week number of date when current build has started.
 * Assume that the start of the week in on Monday.
 */
def get_work_week_of_build_start() {
	// Groovy sandbox does not allow to change properties of locale, default start of the week is on Sunday,
	// so we need to make a trick: shift the starting date by one day ahead - this will give us in the result proper week
	// number with starting on Monday.

	// Groovy sandbox does not allow creation of Date objects with constructor different than Unix milliseconds epoch,
	// so we cannot count in days but only in milliseconds
	// (24 hours / 1 day) * (60 minutes / 1 hour) * (60 seconds / 1 minute) * (1000 milliseconds / 1 second):
	def one_day_in_ms = 24 * 60 * 60 * 1000

	// get build start date object:
	def start_date = new Date(currentBuild.startTimeInMillis + one_day_in_ms)

	// get week number and cast it to integer:
	return Integer.valueOf(start_date.format("w"))
}

class TestsResult {
	Integer all = 0
	Integer passed = 0
	Integer failed = 0
	Integer skipped = 0
	def currentBuild, env


	TestsResult (results = [], def currentBuild, def env) {
		results.each {
			if (it != null) {
				this.all += it.getTotalCount()
				this.passed += it.getPassCount()
				this.failed += it.getFailCount()
				this.skipped += it.getSkipCount()
			}
		}
		this.currentBuild = currentBuild
		this.env = env
	}

	def get_html_table_data_row() {
		def urls =
				"<a href=\"${env.JENKINS_URL}blue/organizations/jenkins/${currentBuild.projectName}/detail/${currentBuild.projectName}/${currentBuild.id}/pipeline\">blue</a>, <a href=\"${env.JENKINS_URL}blue/organizations/jenkins/${currentBuild.projectName}/detail/${currentBuild.projectName}/${currentBuild.id}/tests\">blue tests</a>, <a href=\"${env.JENKINS_URL}view/all/job/${currentBuild.projectName}/${currentBuild.id}/testReport\">tests</a>, <a href=\"${env.JENKINS_URL}view/all/job/${currentBuild.projectName}/${currentBuild.id}/\">build</a>, <a href=\"${env.JENKINS_URL}view/all/job/${currentBuild.projectName}/${currentBuild.id}/consoleText\">console</a>, <a href=\"${env.JENKINS_URL}job/${currentBuild.projectName}/${currentBuild.id}/artifact/results/system_info.txt\">system_info</a>"

		return """
				<tr>
					<td>${currentBuild.projectName}</td>
					<td>${currentBuild.displayName}</td>
					<td>ww${get_work_week_of_build_start()} ${new Date(currentBuild.startTimeInMillis).format("dd.MM.yyyy HH:mm")}</td>
					<td>${currentBuild.durationString}</td>
					<td>${currentBuild.currentResult}</td>
					<td>${urls}</td>
					<td>${this.all}</td>
					<td>${this.passed}</td>
					<td>${this.skipped}</td>
					<td>${this.failed}</td>
				</tr>
		"""
	}

	def get_html_table_heading_row() {
		return """
			<tr>
				<th>Pipeline</th>
				<th>Build</th>
				<th>Starting time</th>
				<th>Duration</th>
				<th>Pipeline status</th>
				<th>URLs</th>
				<th>All tests</th>
				<th>Passed</th>
				<th>Skipped</th>
				<th>Failed</th>
			</tr>
		"""
	}

	def get_html_table() {
		return """
			<table border="1">
				${this.get_html_table_heading_row()}
				${this.get_html_table_data_row()}
			</table>
		"""
	}

}

/**
 * Send summary with test results via email.
 * @param results List of objects returned by `junit` call.
 * @param recipients Addressee of the mail.
 */

def send_test_summary_via_mail(results = [], recipients = params.EMAIL_RECIPIENTS) {
	def message_title = "[Jenkins/PMDK] Report ${currentBuild.projectName} ${currentBuild.displayName}, ww ${get_work_week_of_build_start()}"
	def tests_results = new TestsResult(results, currentBuild, env)
	def message_body =	"""
		${tests_results.get_html_table()}
		<p>---<br />generated automatically by <a href="${env.JENKINS_URL}">Jenkins</a></p>
	"""

	mail (
		to: recipients,
		subject: message_title,
		body: message_body,
		mimeType: "text/html"
	)
}

/**
 * Send summary with test results to webpage: pmdk-val.pact.intel.com
 * @param content Contains test results (html)
 */

def send_test_summary_to_webpage(results = []) { 
	println "Temporary disabled"
	// def k8s_user = 'tiller'
	// def k8s_host = '10.237.156.16'
	// def html_file_name = 'row.html'
	// def credentials_id = 'k8s-tiller'
	// def tests_results = new TestsResult(results, currentBuild, env)
	// def rowBody = tests_results.get_html_table_data_row()

	// writeFile file: html_file_name, text: rowBody
	// withCredentials([sshUserPrivateKey(credentialsId: credentials_id, keyFileVariable: 'keyfile')]) {
	// 	def sendFileCmd = "scp -o 'StrictHostKeyChecking no' -i ${keyfile} ${html_file_name} ${k8s_user}@${k8s_host}:/home/tiller/charts/pmdk-val/static"
	// 	run_bash_script(sendFileCmd)
	// 	run_bash_script_with_common_import("update_website_content_via_ssh ${keyfile} ${k8s_user} ${k8s_host}")
	// }
}

def check_os() {
	distro = sh(script:
	"""#!/usr/bin/env bash
		source ${libs.api.COMMON_FILE}
		check_distro
	""", returnStdout: true).trim()
	return DistroName.from_string(distro)
}

/**
 * Function for applying regular expression to a text.
 * @param text Text to apply regex to - input for a regex engine.
 * @param pattern Regex pattern to be searched by a regex engine.
 * @param groups List of expected group names to be extracted.
 * @param pattern_flags Flags for regex engine. Default: MULTILINE | COMMENTS
 * @return list of dictionaries with all matched groups
 * Example:
 * def input_string = "changed: 10.91.28.117\nok: 0.91.28.119"
 * def pattern = $/ ^(?<status>\w+):\s(?<ip>[\w.]+)$ /$
 * def groups = ["status", "ip"]
 * def result = apply_regex(input_string, pattern, groups)
 * // result will contain list of matches: [[status:changed, ip:10.91.28.117], [status:ok, ip:0.91.28.119]]
 */
@NonCPS
def apply_regex(text, pattern, groups, pattern_flags = Pattern.COMMENTS | Pattern.MULTILINE) {
	Matcher regex_matcher = Pattern.compile(pattern, pattern_flags).matcher(text);
	def found_matches = []
	while (regex_matcher.find()) {
		def current_match = [:]
		for (current_group in groups) {
			current_match[current_group] = regex_matcher.group(current_group)
		}
		found_matches.add(current_match)
	}
	return found_matches
}

// below is necessary in order to closure work properly after loading the file in the pipeline:
return this
