# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
import os

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags'], tooldir=['.waf-tools'])

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags'])

    if not os.environ.has_key('PKG_CONFIG_PATH'):
        os.environ['PKG_CONFIG_PATH'] = ':'.join([
            '/usr/lib/pkgconfig',
            '/usr/local/lib/pkgconfig',
            '/opt/local/lib/pkgconfig'])
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)
    conf.check(lib='dash', uselib="DASH", define_name='HAVE_DASH')
    conf.check(lib='boost_thread', uselibt="BOOST_THREAD", define_name='HAVE_BOOST_THREAD')

def build(bld):
    bld.program(
        features='cxx',
        target='dashproducer',
        source='src/producer/Producer.cpp  src/utils/buffer.cpp',
        use='NDN_CXX BOOST_THREAD',
        )

    #bld.program(
    #    features='cxx',
    #    target='fileconsumer',
    #    source='src/fileconsumer/FileConsumer.cpp',
    #    use='NDN_CXX',
    #    )

    bld.program(
        features='cxx',
        target='dashplayer',
        source='src/dashplayer/dashplayer.cpp src/dashplayer/filedownloader.cpp src/utils/buffer.cpp src/dashplayer/adaptationlogic.cpp src/dashplayer/multimediabuffer.cpp src/dashplayer/adaptationlogic-buffer-svc.cpp src/dashplayer/adaptationlogic-rate-svc.cpp',
        use='NDN_CXX DASH BOOST_THREAD',
        )
