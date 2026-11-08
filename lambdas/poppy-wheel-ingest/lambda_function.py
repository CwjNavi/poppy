"""poppy-wheel-ingest: invoked by the AWS IoT rule on topic 'poppy-wheel'.

Event shape (from infra/iot-rule.sql):
    {"spinCount": <int>, "deviceId": "<mqtt client id>"}
"""


def lambda_handler(event, context):
    # TODO: paste the deployed function code here
    raise NotImplementedError
