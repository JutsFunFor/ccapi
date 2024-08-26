pipeline {
    agent any

    parameters {
        choice(
            name: 'BUILD_TYPE',
            choices: ['APP', 'EXAMPLE'],
            description: 'Выберите тип сборки'
        )
        choice(
            name: 'TARGET',
            choices: [],
            description: 'Выберите целевой исполняемый файл для сборки',
            // Этот параметр будет динамически обновляться
            script: 'return params.BUILD_TYPE == "APP" ? ["single_order_execution", "spot_market_making"] : ["market_data_simple_subscription", "cross_exchange_arbitrage", "custom_service_class", "enable_library_logging", "execution_management_advanced_request", "execution_management_advanced_subscription", "execution_management_simple_request", "execution_management_simple_subscription", "fix_advanced", "fix_simple", "generic_private_request", "generic_public_request", "market_data_advanced_request", "market_data_advanced_subscription", "market_data_simple_request", "market_making", "override_exchange_url_at_runtime", "utility_set_timer"]'
        )
    }

    stages {
        stage('Prepare Environment') {
            steps {
                script {
                    // Устанавливаем переменные на основе BUILD_TYPE
                    env.BUILD_DIR = params.BUILD_TYPE == 'APP' ? "app/build" : "example/build"
                    env.TARGET_PATH = "./${BUILD_DIR}/src/${params.TARGET}/${params.TARGET}"
                }
            }
        }

        stage('Create Build Directory') {
            steps {
                // Создаем папку для сборки, если она не существует
                sh """
                if [ ! -d "${BUILD_DIR}" ]; then
                    mkdir -p ${BUILD_DIR}
                fi
                """
            }
        }

        stage('Run CMake') {
            steps {
                // Переходим в папку сборки и запускаем CMake для генерации Makefile
                sh """
                cd ${BUILD_DIR}
                cmake ..
                """
            }
        }

        stage('Build Project') {
            steps {
                // Сборка проекта с использованием CMake
                sh """
                cd ${BUILD_DIR}
                cmake --build . --target ${params.TARGET}
                """
            }
        }

        stage('Run Executable') {
            steps {
                script {
                    // Проверяем, существует ли скомпилированный файл, и если да, запускаем его
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
