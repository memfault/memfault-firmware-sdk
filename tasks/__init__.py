from invoke import Collection, task

from .esp32 import ESP32, esp32_app_build, esp32_app_clean
from .wiced import WICED, wiced_build, wiced_clean

COLLECTIONS = (ESP32, WICED)

ns = Collection()
for collection in COLLECTIONS:
    ns.add_collection(collection)


@task(pre=[esp32_app_clean, esp32_app_build, wiced_clean, wiced_build])
def build_all_demos(ctx):
    """ Builds all demo apps (for CI purposes) """
    pass


ci = Collection("~ci")
ci.add_task(build_all_demos, name="build-all-demos")
ns.add_collection(ci)
