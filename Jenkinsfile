pipeline {
  agent {
    docker {
      image 'harmonyone/harmony-jenkins-agent:latest'
    }

  }
  stages {
    stage('Configure') {
      steps {
        sh '''mkdir -p build
cd build
cmake ..'''
      }
    }
    stage('Build') {
      steps {
        sh 'make'
      }
    }
    stage('Install') {
      steps {
        sh '''rm -rf destdir
make DESTDIR=`pwd`/destdir install
'''
      }
    }
    stage('Package') {
      steps {
        sh '''find -d destdir/usr/local | \\
sed -n \'s@^destdir/usr/local/@@p\' | \\
tr \'\\n\' \'\\0\' | \\
cpio -o0 -Hnewc -v | \\
xz -9 > mcl.cpio.xz'''
      }
    }
  }
}