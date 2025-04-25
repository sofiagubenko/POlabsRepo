from locust import HttpUser, task, between

class WebsiteUser(HttpUser):
    wait_time = between(1, 2)

    @task(2)
    def load_index(self):
        self.client.get("/index.html")

    @task(1)
    def load_page2(self):
        self.client.get("/page2.html")

    @task(1)
    def load_notfound(self):
        self.client.get("/notfound.html")