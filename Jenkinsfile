pipeline {
  agent {
    docker {
      image 'ubuntu:18.04'
    }

  }
  stages {
    stage('Install Dependencies') {
      steps {
        sh '''apt-get install -y \\
    libssl-dev libgmp-dev \\
    gcc g++ cmake make \\
    cpio xz-utils'''
      }
    }
    stage('Configure') {
      steps {
        sh '''mkdir build && cd build && cmake ..
'''
      }
    }
    stage('Build') {
      steps {
        sh 'make'
      }
    }
    stage('Install') {
      steps {
        sh '''list() { (
    cd /usr/local && \\
    find . | sed -n \'s@^\\./@@p\' | sort
) }
list > before.txt
make install
list > after.txt
comm -13 before.txt after.txt > installed.txt'''
        }
      }
      stage('Package') {
        steps {
          sh '''sort -r -t/ installed.txt | \\
tr \'\\n\' \'\\0\' | \\
cpio -o0 -Hnewc | \\
xz -9 > mcl.cpio.xz'''
        }
      }
    }
  }