pipeline {
    agent any

    parameters {
        choice(
            name: 'TARGET',
            choices: ['APP', 'EXAMPLE'],
            description: 'Выберите тип сборки'
        )
    }

    stages {
        stage('Choose Specific Target') {
            steps {
                script {
                    if (params.TARGET == 'APP') {
                        env.BUILD_TARGET = input(
                            message: 'Выберите целевой исполняемый файл для сборки (APP)',
                            parameters: [
                                choice(name: 'BUILD_TARGET', choices: ['single_order_execution', 'spot_market_making'], description: 'Целевой файл для сборки')
                            ]
                        )
                    } else if (params.TARGET == 'EXAMPLE') {
                        env.BUILD_TARGET = input(
                            message: 'Выберите целевой исполняемый файл для сборки (EXAMPLE)',
                            parameters: [
                                choice(name: 'BUILD_TARGET', choices: [
                                    'market_data_simple_subscription',
                                    'cross_exchange_arbitrage',
                                    'custom_service_class',
                                    'enable_library_logging',
                                    'execution_management_advanced_request',
                                    'execution_management_advanced_subscription',
                                    'execution_management_simple_request',
                                    'execution_management_simple_subscription',
                                    'fix_advanced',
                                    'fix_simple',
                                    'generic_private_request',
                                    'generic_public_request',
                                    'market_data_advanced_request',
                                    'market_data_advanced_subscription',
                                    'market_data_simple_request',
                                    'market_making',
                                    'override_exchange_url_at_runtime',
                                    'utility_set_timer'
                                ], description: 'Целевой файл для сборки')
                            ]
                        )
                    }
                    env.BUILD_DIR = params.TARGET == 'APP' ? "app/build" : "example/build"
                    env.TARGET_PATH = "./${BUILD_DIR}/src/${env.BUILD_TARGET}/${env.BUILD_TARGET}"
                }
            }
        }

        stage('Create Build Directory') {
            steps {
                sh """
                if [ ! -d "${BUILD_DIR}" ]; then
                    mkdir -p ${BUILD_DIR}
                fi
                """
            }
        }

        stage('Run CMake') {
            steps {
                sh """
                cd ${BUILD_DIR}
                cmake ..
                """
            }
        }

        stage('Build Project') {
            steps {
                sh """
                cd ${BUILD_DIR}
                cmake --build . --target ${env.BUILD_TARGET}
                """
            }
        }

        stage('Run Executable') {
            steps {
                script {
                    sh """
                    if [ -f "${TARGET_PATH}" ]; then
                        echo "Приложение скомпилировано ${TARGET_PATH}..."
                    else
                        echo "Ошибка: файл ${TARGET_PATH} не найден."
                        exit 1
                    fi
                    """
                }
            }
        }
    }

    post {
        failure {
            echo 'Сборка завершилась с ошибкой.'
        }
        success {
            echo 'Сборка и запуск завершены успешно.'
        }
    }
}
