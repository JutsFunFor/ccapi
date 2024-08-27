pipeline {
    agent any

    stages {
        stage('Prepare Environment') {
            steps {
                script {
                    // Set the build directory based on the TARGET
                    env.BUILD_DIR = params.TARGET == 'APP' ? "app/build" : "example/build"
                    // Define the path to the target executable
                    env.TARGET_PATH = "./${BUILD_DIR}/src/${params.BUILD_TARGET}/${params.BUILD_TARGET}"
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
                cmake --build . --target ${params.BUILD_TARGET}
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
