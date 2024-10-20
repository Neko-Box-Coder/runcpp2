def bash(command, returnOutput=false)
{
    return sh(script: """#!/bin/bash
        ${command}
        """, 
        returnStdout: returnOutput)
}

def SetGithubStatus(githubToken, context, targetUrl, desc, status, repoOwner, repoName, gitHash)
{
    bash """
        curl -L --fail-with-body \\
        -X POST \\
        -H "Accept: application/vnd.github+json" \\
        -H "Authorization: Bearer ${githubToken}" \\
        -H "X-GitHub-Api-Version: 2022-11-28" \\
        -d '{
            "context": "${context}", 
            "target_url": "${targetUrl}",
            "description": "${desc}",
            "state": "${status}"
        }' \\
        https://api.github.com/repos/${repoOwner}/${repoName}/statuses/${gitHash}
        """
}

def REPO_OWNER = "Neko-Box-Coder"
def REPO_NAME = "runcpp2"
def TARGET_URL = 'https://github.com/Neko-Box-Coder/runcpp2.git'
def STATUS_CONTEXT_URL = "https://ci.nekoboxcoder.dev/job/runcpp2/${BUILD_NUMBER}/pipeline-graph/"
def FAILED_STAGE = "None"
def TARGET_REF = "refs/heads/master"
def STORE_BUILD = false

pipeline 
{
    agent none
    options { skipDefaultCheckout() }
/*
    External Variables for webhook payload:
    
    Webhook:
        GITHUB_PUSH_REF: $.ref
        GITHUB_PR_ACTION: $.action
        GITHUB_PR_GIT_URL: $.pull_request.head.repo.clone_url
        GITHUB_PR_REF: $.pull_request.head.ref
        GITHUB_PR_REPO_OWNER: $.pull_request.user.login
        GITHUB_PR_REPO_NAME: $.pull_request.head.repo.name
        X-GitHub-Event: (Header)
    
    Trigger:
        $GITHUB_PUSH_REF , $GITHUB_PR_REF , $GITHUB_PR_ACTION
        ^refs/heads/master , , $|^ , .[a-zA-Z/]* , (opened|synchronize)$

    Param:
        TARGET_REF
        STORE_BUILD
*/
    stages 
    {
        stage('Setup') 
        {
            agent none
            steps 
            {
                script
                {
                    echo "env.TARGET_REF: ${env.TARGET_REF}"
                    echo "env.STORE_BUILD: ${env.STORE_BUILD}"
                    
                    if(env.TARGET_REF != null)
                        TARGET_REF = env.TARGET_REF
                    
                    if(env.STORE_BUILD != null)
                        STORE_BUILD = env.STORE_BUILD.toBoolean()
                    
                    echo "Displaying Webhook Variables:"
                    echo "GITHUB_PUSH_REF: ${env.GITHUB_PUSH_REF}"
                    echo "GITHUB_PR_ACTION: ${env.GITHUB_PR_ACTION}"
                    echo "GITHUB_PR_GIT_URL: ${env.GITHUB_PR_GIT_URL}"
                    echo "GITHUB_PR_REF: ${env.GITHUB_PR_REF}"
                    echo "GITHUB_PR_REPO_OWNER: ${env.GITHUB_PR_REPO_OWNER}"
                    echo "GITHUB_PR_REPO_NAME: ${env.GITHUB_PR_REPO_NAME}"
                    echo "X_GitHub_Event: ${env.X_GitHub_Event}"
                    
                    //Trigger pipeline on push to master
                    if(env.X_GitHub_Event == 'push')
                    {
                        if(env.GITHUB_PUSH_REF != 'refs/heads/master')
                            error('Receiving non master push')
                    }
                    //Trigger pipeline with approval on PR
                    else if(env.X_GitHub_Event == 'pull_request')
                    {
                        if(env.GITHUB_PR_ACTION != 'synchronize' && env.GITHUB_PR_ACTION != 'opened')
                            error('Receiving non relevant PR action')
                        else
                        {
                            timeout(time: 30, unit: 'MINUTES')
                            {
                                input 'Approval this job?'
                            }
                        }
                        
                        TARGET_REF = env.GITHUB_PR_REF
                        TARGET_URL = env.GITHUB_PR_GIT_URL
                        REPO_OWNER = env.GITHUB_PR_REPO_OWNER
                        REPO_NAME = env.GITHUB_PR_REPO_NAME
                    }
                    //Invalid github event
                    else if(env.X_GitHub_Event != null)
                        error("Invalid github event: ${env.X_GitHub_Event}")
                    
                    echo "TARGET_REF: ${TARGET_REF}"
                    echo "env.TARGET_REF: ${env.TARGET_REF}"
                    echo "TARGET_URL: ${TARGET_URL}"
                    echo "REPO_OWNER: ${REPO_OWNER}"
                    echo "REPO_NAME: ${REPO_NAME}"
                    echo "STATUS_CONTEXT_URL: ${STATUS_CONTEXT_URL}"
                    echo "STORE_BUILD: ${STORE_BUILD}"
                }
            }
        }

        stage('Checkout') 
        {
            agent { label 'linux' }
            steps 
            {
                cleanWs()
                script
                {
                    checkout(
                        [
                            $class: 'GitSCM',
                            branches: [[name: TARGET_REF]],
                            doGenerateSubmoduleConfigurations: true,
                            extensions: scm.extensions + 
                            [[
                                $class: 'SubmoduleOption', 
                                parentCredentials: true,
                                recursiveSubmodules: true
                            ]],
                            userRemoteConfigs: [[url: TARGET_URL]]
                        ]
                    )
                    
                    GIT_HASH = bash("echo \$(git rev-parse --verify HEAD)", true)
                    echo "GITHASH: ${GIT_HASH}"
                    
                    if(!STORE_BUILD)
                    {
                        withCredentials([string(credentialsId: 'github-token', 
                                                variable: 'GITHUB_TOKEN')])
                        {
                            SetGithubStatus('$GITHUB_TOKEN', 
                                            "Build ${BUILD_NUMBER}", 
                                            STATUS_CONTEXT_URL, 
                                            "Stage ${env.STAGE_NAME} started",
                                            "pending",
                                            REPO_OWNER,
                                            REPO_NAME,
                                            GIT_HASH)
                        }
                    }
                    
                    stash 'source'
                }
            }
            post { failure { script { FAILED_STAGE = env.STAGE_NAME } } }
        }

        stage('Build') 
        {
            parallel 
            {
                stage('Linux Build') 
                {
                    agent { label 'linux' }
                    steps 
                    {
                        cleanWs()
                        bash "ls -lah"
                        unstash 'source'
                        bash "ls -lah"
                        bash('chmod +x ./Build.sh')
                        bash './Build.sh -DRUNCPP2_BUILD_TESTS=ON'
                        stash 'linux_build'
                    }
                    post { failure { script { FAILED_STAGE = env.STAGE_NAME } } }
                }
                stage('Windows Build') 
                {
                    agent { label 'windows' }
                    steps 
                    {
                        cleanWs()
                        bat 'dir'
                        unstash 'source'
                        bat 'dir'
                        bat 'Build.bat -DRUNCPP2_BUILD_TESTS=ON'
                        stash 'windows_build'
                    }
                    post { failure { script { FAILED_STAGE = env.STAGE_NAME } } }
                }
            }
            
            //NOTE: We use Debug builds for now even for release.
        }

        stage('Test') 
        {
            parallel 
            {
                stage('Linux Test') 
                {
                    agent { label 'linux' }
                    steps 
                    {
                        cleanWs()
                        bash "ls -lah"
                        unstash 'linux_build'
                        bash "ls -lah"
                        bash './Build/BuildsManagerTest'
                    }
                    post { failure { script { FAILED_STAGE = env.STAGE_NAME } } }
                }
                stage('Windows Test') 
                {
                    agent { label 'windows' }
                    steps 
                    {
                        cleanWs()
                        bat 'dir'
                        unstash 'windows_build'
                        bat 'dir'
                        bat '.\\Build\\Debug\\BuildsManagerTest.exe'
                    }
                    post { failure { script { FAILED_STAGE = env.STAGE_NAME } } }
                }
            }
        }
        
        stage('Notify')
        {
            agent { label 'linux' }
            when { expression { STORE_BUILD == false } }
            steps 
            {
                cleanWs()
                withCredentials([string(credentialsId: 'github-token', variable: 'GITHUB_TOKEN')])
                {
                    SetGithubStatus('$GITHUB_TOKEN', 
                                    "Build ${BUILD_NUMBER}", 
                                    STATUS_CONTEXT_URL, 
                                    "Pipeline passed",
                                    "success",
                                    REPO_OWNER,
                                    REPO_NAME,
                                    GIT_HASH)
                }
            }
        }
        
        stage('Release')
        {
            agent { label 'linux' }
            when { expression { STORE_BUILD == true } }
            steps
            {
                cleanWs()
                dir('WindowsBuild') { unstash 'windows_build' }
                dir('LinuxBuild') { unstash 'linux_build' }
                
                archiveArtifacts    artifacts: 'LinuxBuild/Build/runcpp2', 
                                    defaultExcludes: false,
                                    fingerprint: true,
                                    onlyIfSuccessful: true
                
                archiveArtifacts    artifacts: 'WindowsBuild/Build/Debug/runcpp2.exe', 
                                    defaultExcludes: false,
                                    fingerprint: true,
                                    onlyIfSuccessful: true
            }
        }
    } //stages
    
    post 
    {
        failure 
        {
            node('linux')
            {
                script
                {
                    if(!STORE_BUILD)
                    {
                        withCredentials([string(credentialsId: 'github-token', variable: 'GITHUB_TOKEN')])
                        {
                            SetGithubStatus('$GITHUB_TOKEN', 
                                            "Build ${BUILD_NUMBER}", 
                                            STATUS_CONTEXT_URL, 
                                            "Stage ${FAILED_STAGE} failed",
                                            "failure",
                                            REPO_OWNER,
                                            REPO_NAME,
                                            GIT_HASH)
                        }
                    }
                }
            }
        }
    }
} //pipeline
