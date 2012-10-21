Worker Basics
*************

Workers take "work" in the form of messages sent by clients to the server. Clients call workers via a call to a "function". Whether or not message content is provided to the "function" is entirely left to the design of the end user. Message content is limited to ~4gigs (though the practical size is based on the amount of memory available to the server, so this number is likely to be much smaller).

The server takes the work sent to it, and then passes it along to the first available worker to process. The work then returns a response in a single message, or it sends the message as "chunks" to the client. Clients by default receive messages as a single reply, so no work is needed to support "chunked" replies. A worker does not have to send a message in response, it can just return whether or not the work was carried out.

As of 0.32 the following are the responses from a worker that is supported:

:c:type:`GEARMAN_SUCCESS`

On success the client is sent the result, and the work is marked as successfully completed.

:c:type:`GEARMAN_FAIL`

Of fail the server will report to the client that an error occurred and the work will not be retried. No result will be sent to the client.

:c:type:`GEARMAN_ERROR`

On error the server will retry the work, until job retries occurs (ie gearmand --job-retries=#)

:c:type:`GEARMAN_SHUTDOWN`

A shutdown message causes gearman_worker_work exit from its loop. The client is sent :c:type:`GEARMAN_SUCCESS`, and the result is sent to the client.
