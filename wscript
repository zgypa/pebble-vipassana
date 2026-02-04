from pebble_sdk import pebble


def options(ctx):
    pebble.init_options(ctx)


def configure(ctx):
    pebble.init_configuration(ctx)


def build(ctx):
    pebble.build(ctx)
