def bash(command)
{
    return sh("""#!/bin/bash
        ${command}
        """)
}

pipeline 
{
    agent none
    
    options
    {
        skipDefaultCheckout()
    }

/*
    environment
    {
        // GITHUB_WEBHOOK_TOKEN = credentials('github_webhook_token')
    }
    triggers 
    {
        GenericTrigger(
            genericVariables: 
            [
                [key: 'ref', value: '$.ref'],
                [key: 'action', value: '$.action']
            ],
            causeString: 'Triggered on $ref',
            tokenCredentialId: "github_webhook_token",
            printContributedVariables: true,
            printPostContent: true,
            silentResponse: false
        )
    }
*/

    stages 
    {
        stage('Checkout') 
        {
            agent { label 'linux' }
            steps 
            {
                cleanWs()
                script
                {
                    echo "ref: ${env.ref}"
                    echo "action: ${env.action}"
                    bash "ls -lah"
                    
                    checkout(
                        [
                            $class: 'GitSCM',
                            branches: scm.branches,
                            doGenerateSubmoduleConfigurations: true,
                            extensions: scm.extensions + 
                            [[
                                $class: 'SubmoduleOption', 
                                parentCredentials: true,
                                recursiveSubmodules: true
                            ]],
                            userRemoteConfigs: scm.userRemoteConfigs
                        ]
                    )
                    
                    stash 'source'
                }
            }
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
                        bash('chmod +x ./Build.sh')
                        bash './Build.sh -DRUNCPP2_BUILD_TESTS=ON'
                        stash 'linux_build'
                    }
                }
                stage('Windows Build') 
                {
                    agent { label 'windows' }
                    steps 
                    {
                        cleanWs()
                        bat 'dir'
                        unstash 'source'
                        bat 'Build.bat -DRUNCPP2_BUILD_TESTS=ON'
                        stash 'windows_build'
                    }
                }
            }
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
                }
            }
        }
    }

    post 
    {
        always 
        {
            node('linux') 
            {
                cleanWs()
            }
            node('windows') 
            {
                cleanWs()
            }
        }
    }
}
