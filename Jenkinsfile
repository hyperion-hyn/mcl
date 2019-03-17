pipeline {
  agent {
    docker {
      image 'harmonyone/harmony-jenkins-agent:latest'
    }

  }
  stages {
    stage('Build & Install') {
      steps {
        sh '''
          rm -rf destdir
          install -d destdir/usr/local/include
          install -d destdir/usr/local/lib
          make PREFIX=`pwd`/destdir/usr/local install
        '''
      }
    }
    stage('Package') {
      steps {
        sh '''
          find destdir/usr/local -depth | \\
            sed -n \'s@^destdir/usr/local/@@p\' | \\
            tr \'\\n\' \'\\0\' | \\
            (cd destdir/usr/local && exec cpio -o0 -Hnewc -v) | \\
            xz -9 > mcl.cpio.xz
        '''
      }
    }
    stage('Save Build') {
      steps {
        archiveArtifacts '**'
      }
    }
  }
}
